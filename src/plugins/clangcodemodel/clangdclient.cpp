/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangdclient.h"

#include <coreplugin/find/searchresultitem.h>
#include <coreplugin/find/searchresultwindow.h>
#include <cplusplus/FindUsages.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppfindreferences.h>
#include <cpptools/cpptoolsreuse.h>
#include <languageclient/languageclientinterface.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <texteditor/basefilefind.h>
#include <utils/algorithm.h>

#include <QCheckBox>
#include <QFile>
#include <QHash>
#include <QPointer>
#include <QRegularExpression>

using namespace CPlusPlus;
using namespace Core;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace ProjectExplorer;

namespace ClangCodeModel {
namespace Internal {

static Q_LOGGING_CATEGORY(clangdLog, "qtc.clangcodemodel.clangd", QtWarningMsg);
static QString indexingToken() { return "backgroundIndexProgress"; }

class AstParams : public JsonObject
{
public:
    AstParams() {}
    AstParams(const TextDocumentIdentifier &document, const Range &range);
    using JsonObject::JsonObject;

    // The open file to inspect.
    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &id) { insert(textDocumentKey, id); }

    // The region of the source code whose AST is fetched. The highest-level node that entirely
    // contains the range is returned.
    Utils::optional<Range> range() const { return optionalValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    bool isValid() const override { return contains(textDocumentKey); }
};

class AstNode : public JsonObject
{
public:
    using JsonObject::JsonObject;

    static constexpr char roleKey[] = "role";
    static constexpr char arcanaKey[] = "arcana";

    // The general kind of node, such as “expression”. Corresponds to clang’s base AST node type,
    // such as Expr. The most common are “expression”, “statement”, “type” and “declaration”.
    QString role() const { return typedValue<QString>(roleKey); }

    // The specific kind of node, such as “BinaryOperator”. Corresponds to clang’s concrete
    // node class, with Expr etc suffix dropped.
    QString kind() const { return typedValue<QString>(kindKey); }

    // Brief additional details, such as ‘||’. Information present here depends on the node kind.
    Utils::optional<QString> detail() const { return optionalValue<QString>(detailKey); }

    // One line dump of information, similar to that printed by clang -Xclang -ast-dump.
    // Only available for certain types of nodes.
    Utils::optional<QString> arcana() const { return optionalValue<QString>(arcanaKey); }

    // The part of the code that produced this node. Missing for implicit nodes, nodes produced
    // by macro expansion, etc.
    Range range() const { return typedValue<Range>(rangeKey); }

    // Descendants describing the internal structure. The tree of nodes is similar to that printed
    // by clang -Xclang -ast-dump, or that traversed by clang::RecursiveASTVisitor.
    Utils::optional<QList<AstNode>> children() const { return optionalArray<AstNode>(childrenKey); }

    bool hasRange() const { return contains(rangeKey); }

    bool arcanaContains(const QString &s) const
    {
        const Utils::optional<QString> arcanaString = arcana();
        return arcanaString && arcanaString->contains(s);
    }

    bool detailIs(const QString &s) const
    {
        return detail() && detail().value() == s;
    }

    QString type() const
    {
        const Utils::optional<QString> arcanaString = arcana();
        if (!arcanaString)
            return {};
        const int quote1Offset = arcanaString->indexOf('\'');
        if (quote1Offset == -1)
            return {};
        const int quote2Offset = arcanaString->indexOf('\'', quote1Offset + 1);
        if (quote2Offset == -1)
            return {};
        return arcanaString->mid(quote1Offset + 1, quote2Offset - quote1Offset - 1);
    }

    // Returns true <=> the type is "recursively const".
    // E.g. returns true for "const int &", "const int *" and "const int * const *",
    // and false for "int &" and "const int **".
    // For non-pointer types such as "int", we check whether they are uses as lvalues
    // or rvalues.
    bool hasConstType() const
    {
        QString theType = type();
        if (theType.endsWith("const"))
            theType.chop(5);
        const int ptrRefCount = theType.count('*') + theType.count('&');
        const int constCount = theType.count("const");
        if (ptrRefCount == 0)
            return constCount > 0 || detailIs("LValueToRValue");
        return ptrRefCount <= constCount;
    }

    bool childContainsRange(int index, const Range &range) const
    {
        const Utils::optional<QList<AstNode>> childList = children();
        return childList && childList->size() > index
                && childList->at(index).range().contains(range);
    }

    QString operatorString() const
    {
        if (kind() == "BinaryOperator")
            return detail().value_or(QString());
        QTC_ASSERT(kind() == "CXXOperatorCall", return {});
        const Utils::optional<QString> arcanaString = arcana();
        if (!arcanaString)
            return {};
        const int closingQuoteOffset = arcanaString->lastIndexOf('\'');
        if (closingQuoteOffset <= 0)
            return {};
        const int openingQuoteOffset = arcanaString->lastIndexOf('\'', closingQuoteOffset - 1);
        if (openingQuoteOffset == -1)
            return {};
        return arcanaString->mid(openingQuoteOffset + 1, closingQuoteOffset
                                 - openingQuoteOffset - 1);
    }

    bool isValid() const override
    {
        return contains(roleKey) && contains(kindKey);
    }
};

static QList<AstNode> getAstPath(const AstNode &root, const Range &range)
{
    QList<AstNode> path;
    QList<AstNode> queue{root};
    bool isRoot = true;
    while (!queue.isEmpty()) {
        AstNode curNode = queue.takeFirst();
        if (!isRoot && !curNode.hasRange())
            continue;
        if (curNode.range() == range)
            return path << curNode;
        if (isRoot || curNode.range().contains(range)) {
            path << curNode;
            const auto children = curNode.children();
            if (!children)
                break;
            queue = children.value();
        }
        isRoot = false;
    }
    return path;
}

static Usage::Type getUsageType(const QList<AstNode> &path)
{
    bool potentialWrite = false;
    const bool symbolIsDataType = path.last().role() == "type" && path.last().kind() == "Record";
    for (auto pathIt = path.rbegin(); pathIt != path.rend(); ++pathIt) {
        if (pathIt->arcanaContains("non_odr_use_unevaluated"))
            return Usage::Type::Other;
        if (pathIt->kind() == "CXXDelete")
            return Usage::Type::Write;
        if (pathIt->kind() == "CXXNew")
            return Usage::Type::Other;
        if (pathIt->kind() == "Switch" || pathIt->kind() == "If")
            return Usage::Type::Read;
        if (pathIt->kind() == "Call" || pathIt->kind() == "CXXMemberCall")
            return potentialWrite ? Usage::Type::WritableRef : Usage::Type::Read;
        if ((pathIt->kind() == "DeclRef" || pathIt->kind() == "Member")
                && pathIt->arcanaContains("lvalue")) {
            potentialWrite = true;
        }
        if (pathIt->role() == "declaration") {
            if (symbolIsDataType)
                return Usage::Type::Other;
            if (pathIt->arcanaContains("cinit")) {
                if (pathIt == path.rbegin())
                    return Usage::Type::Initialization;
                if (pathIt->childContainsRange(0, path.last().range()))
                    return Usage::Type::Initialization;
                if (!pathIt->hasConstType())
                    return Usage::Type::WritableRef;
                return Usage::Type::Read;
            }
            return Usage::Type::Declaration;
        }
        if (pathIt->kind() == "MemberInitializer")
            return pathIt == path.rbegin() ? Usage::Type::Write : Usage::Type::Read;
        if (pathIt->kind() == "UnaryOperator"
                && (pathIt->detailIs("++") || pathIt->detailIs("--"))) {
            return Usage::Type::Write;
        }

        // LLVM uses BinaryOperator only for built-in types; for classes, CXXOperatorCall
        // is used. The latter has an additional node at index 0, so the left-hand side
        // of an assignment is at index 1.
        const bool isBinaryOp = pathIt->kind() == "BinaryOperator";
        const bool isOpCall = pathIt->kind() == "CXXOperatorCall";
        if (isBinaryOp || isOpCall) {
            if (isOpCall && symbolIsDataType) // Constructor invocation.
                return Usage::Type::Other;

            const QString op = pathIt->operatorString();
            if (op.endsWith("=") && op != "==") { // Assignment.
                const int lhsIndex = isBinaryOp ? 0 : 1;
                if (pathIt->childContainsRange(lhsIndex, path.last().range()))
                    return Usage::Type::Write;
                return potentialWrite ? Usage::Type::WritableRef : Usage::Type::Read;
            }
            return Usage::Type::Read;
        }

        if (pathIt->kind() == "ImplicitCast") {
            if (pathIt->detailIs("FunctionToPointerDecay"))
                return Usage::Type::Other;
            if (pathIt->hasConstType())
                return Usage::Type::Read;
            potentialWrite = true;
            continue;
        }
    }

    return Usage::Type::Other;
}

class AstRequest : public Request<AstNode, std::nullptr_t, AstParams>
{
public:
    using Request::Request;
    explicit AstRequest(const AstParams &params) : Request("textDocument/ast", params) {}
};

static BaseClientInterface *clientInterface(const Utils::FilePath &jsonDbDir)
{
    Utils::CommandLine cmd{CppTools::codeModelSettings()->clangdFilePath(),
                           {"--background-index", "--limit-results=0"}};
    if (!jsonDbDir.isEmpty())
        cmd.addArg("--compile-commands-dir=" + jsonDbDir.toString());
    if (clangdLog().isDebugEnabled())
        cmd.addArgs({"--log=verbose", "--pretty"});
    const auto interface = new StdIOClientInterface;
    interface->setCommandLine(cmd);
    return interface;
}

class ReferencesFileData {
public:
    QList<QPair<Range, QString>> rangesAndLineText;
    QString fileContent;
    AstNode ast;
};
class ReplacementData {
public:
    QString oldSymbolName;
    QString newSymbolName;
    QSet<Utils::FilePath> fileRenameCandidates;
};
class ReferencesData {
public:
    QMap<DocumentUri, ReferencesFileData> fileData;
    QList<MessageId> pendingAstRequests;
    QPointer<SearchResult> search;
    Utils::optional<ReplacementData> replacementData;
    quint64 key;
    bool canceled = false;
};

class ClangdClient::Private
{
public:
    Private(ClangdClient *q) : q(q) {}

    void handleFindUsagesResult(quint64 key, const QList<Location> &locations);
    static void handleRenameRequest(const SearchResult *search,
                                    const ReplacementData &replacementData,
                                    const QString &newSymbolName,
                                    const QList<Core::SearchResultItem> &checkedItems,
                                    bool preserveCase);
    void addSearchResultsForFile(ReferencesData &refData, const Utils::FilePath &file,
                                 const ReferencesFileData &fileData);
    void reportAllSearchResultsAndFinish(ReferencesData &data);
    void finishSearch(const ReferencesData &refData, bool canceled);

    ClangdClient * const q;
    QHash<quint64, ReferencesData> runningFindUsages;
    Utils::optional<QVersionNumber> versionNumber;
    quint64 nextFindUsagesKey = 0;
    bool isFullyIndexed = false;
    bool isTesting = false;
};

ClangdClient::ClangdClient(Project *project, const Utils::FilePath &jsonDbDir)
    : Client(clientInterface(jsonDbDir)), d(new Private(this))
{
    setName(tr("clangd"));
    LanguageFilter langFilter;
    langFilter.mimeTypes = QStringList{"text/x-chdr", "text/x-c++hdr", "text/x-c++src",
            "text/x-objc++src", "text/x-objcsrc"};
    setSupportedLanguage(langFilter);
    LanguageServerProtocol::ClientCapabilities caps = Client::defaultClientCapabilities();
    caps.clearExperimental();
    caps.clearTextDocument();
    setClientCapabilities(caps);
    setLocatorsEnabled(false);
    setProgressTitleForToken(indexingToken(), tr("Parsing C/C++ Files (clangd)"));
    setCurrentProject(project);
    connect(this, &Client::workDone, this, [this](const ProgressToken &token) {
        const QString * const val = Utils::get_if<QString>(&token);
        if (val && *val == indexingToken()) {
            d->isFullyIndexed = true;
            emit indexingFinished();
        }
    });

    connect(this, &Client::initialized, this, [this] {
        // If we get this signal while there are pending searches, it means that
        // the client was re-initialized, i.e. clangd crashed.

        // Report all search results found so far.
        for (quint64 key : d->runningFindUsages.keys())
            d->reportAllSearchResultsAndFinish(d->runningFindUsages[key]);
        QTC_CHECK(d->runningFindUsages.isEmpty());
    });

    start();
}

ClangdClient::~ClangdClient()
{
    delete d;
}

bool ClangdClient::isFullyIndexed() const { return d->isFullyIndexed; }

void ClangdClient::openExtraFile(const Utils::FilePath &filePath, const QString &content)
{
    QFile cxxFile(filePath.toString());
    if (content.isEmpty() && !cxxFile.open(QIODevice::ReadOnly))
        return;
    TextDocumentItem item;
    item.setLanguageId("cpp");
    item.setUri(DocumentUri::fromFilePath(filePath));
    item.setText(!content.isEmpty() ? content : QString::fromUtf8(cxxFile.readAll()));
    item.setVersion(0);
    sendContent(DidOpenTextDocumentNotification(DidOpenTextDocumentParams(item)));
}

void ClangdClient::closeExtraFile(const Utils::FilePath &filePath)
{
    sendContent(DidCloseTextDocumentNotification(DidCloseTextDocumentParams(
            TextDocumentIdentifier{DocumentUri::fromFilePath(filePath)})));
}

void ClangdClient::findUsages(TextEditor::TextDocument *document, const QTextCursor &cursor,
                              const Utils::optional<QString> &replacement)
{
    QTextCursor termCursor(cursor);
    termCursor.select(QTextCursor::WordUnderCursor);
    const QString searchTerm = termCursor.selectedText(); // TODO: This will be wrong for e.g. operators. Use a Symbol info request to get the real symbol string.
    if (searchTerm.isEmpty())
        return;

    ReferencesData refData;
    refData.key = d->nextFindUsagesKey++;
    if (replacement) {
        ReplacementData replacementData;
        replacementData.oldSymbolName = searchTerm;
        replacementData.newSymbolName = *replacement;
        if (replacementData.newSymbolName.isEmpty())
            replacementData.newSymbolName = replacementData.oldSymbolName;
        refData.replacementData = replacementData;
    }
    refData.search = SearchResultWindow::instance()->startNewSearch(
                tr("C++ Usages:"),
                {},
                searchTerm,
                replacement ? SearchResultWindow::SearchAndReplace : SearchResultWindow::SearchOnly,
                SearchResultWindow::PreserveCaseDisabled,
                "CppEditor");
    refData.search->setFilter(new CppTools::CppSearchResultFilter);
    if (refData.replacementData) {
        refData.search->setTextToReplace(refData.replacementData->newSymbolName);
        const auto renameFilesCheckBox = new QCheckBox;
        renameFilesCheckBox->setVisible(false);
        refData.search->setAdditionalReplaceWidget(renameFilesCheckBox);
        const auto renameHandler =
                [search = refData.search](const QString &newSymbolName,
                                          const QList<SearchResultItem> &checkedItems,
                                          bool preserveCase) {
            const auto replacementData = search->userData().value<ReplacementData>();
            Private::handleRenameRequest(search, replacementData, newSymbolName, checkedItems,
                                         preserveCase);
        };
        connect(refData.search, &SearchResult::replaceButtonClicked, renameHandler);
    }
    connect(refData.search, &SearchResult::activated, [](const SearchResultItem& item) {
        Core::EditorManager::openEditorAtSearchResult(item);
    });
    SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
    d->runningFindUsages.insert(refData.key, refData);

    const Utils::optional<MessageId> requestId = symbolSupport().findUsages(
                document, cursor, [this, key = refData.key](const QList<Location> &locations) {
        d->handleFindUsagesResult(key, locations);
    });

    if (!requestId) {
        d->finishSearch(refData, false);
        return;
    }
    connect(refData.search, &SearchResult::cancelled, this, [this, requestId, key = refData.key] {
        const auto refData = d->runningFindUsages.find(key);
        if (refData == d->runningFindUsages.end())
            return;
        cancelRequest(*requestId);
        refData->canceled = true;
        refData->search->disconnect(this);
        d->finishSearch(*refData, true);
    });
}

void ClangdClient::enableTesting() { d->isTesting = true; }

QVersionNumber ClangdClient::versionNumber() const
{
    if (d->versionNumber)
        return d->versionNumber.value();

    const QRegularExpression versionPattern("^clangd version (\\d+)\\.(\\d+)\\.(\\d+).*$");
    QTC_CHECK(versionPattern.isValid());
    const QRegularExpressionMatch match = versionPattern.match(serverVersion());
    if (match.isValid()) {
        d->versionNumber.emplace({match.captured(1).toInt(), match.captured(2).toInt(),
                                 match.captured(3).toInt()});
    } else {
        qCWarning(clangdLog) << "Failed to parse clangd server string" << serverVersion();
        d->versionNumber.emplace({0});
    }
    return d->versionNumber.value();
}

void ClangdClient::Private::handleFindUsagesResult(quint64 key, const QList<Location> &locations)
{
    const auto refData = runningFindUsages.find(key);
    if (refData == runningFindUsages.end())
        return;
    if (!refData->search || refData->canceled) {
        finishSearch(*refData, true);
        return;
    }
    refData->search->disconnect(q);

    qCDebug(clangdLog) << "found" << locations.size() << "locations";
    if (locations.isEmpty()) {
        finishSearch(*refData, false);
        return;
    }

    QObject::connect(refData->search, &SearchResult::cancelled, q, [this, key] {
        const auto refData = runningFindUsages.find(key);
        if (refData == runningFindUsages.end())
            return;
        refData->canceled = true;
        refData->search->disconnect(q);
        for (const MessageId &id : qAsConst(refData->pendingAstRequests))
            q->cancelRequest(id);
        refData->pendingAstRequests.clear();
        finishSearch(*refData, true);
    });

    for (const Location &loc : locations) // TODO: Can contain duplicates. Rather fix in clang than work around it here.
        refData->fileData[loc.uri()].rangesAndLineText << qMakePair(loc.range(), QString()); // TODO: Can we assume that locations for the same file are grouped?
    for (auto it = refData->fileData.begin(); it != refData->fileData.end(); ++it) {
        const QStringList lines = SymbolSupport::getFileContents(it.key().toFilePath());
        it->fileContent = lines.join('\n');
        for (auto &rangeWithText : it.value().rangesAndLineText) {
            const int lineNo = rangeWithText.first.start().line();
            if (lineNo >= 0 && lineNo < lines.size())
                rangeWithText.second = lines.at(lineNo);
        }
    }

    qCDebug(clangdLog) << "document count is" << refData->fileData.size();
    if (refData->replacementData || q->versionNumber() < QVersionNumber(13)
            || refData->fileData.size() > 15) { // TODO: If we need to keep this, make it configurable.
        qCDebug(clangdLog) << "skipping AST retrieval";
        reportAllSearchResultsAndFinish(*refData);
        return;
    }

    for (auto it = refData->fileData.begin(); it != refData->fileData.end(); ++it) {
        const bool extraOpen = !q->documentForFilePath(it.key().toFilePath());
        if (extraOpen)
            q->openExtraFile(it.key().toFilePath(), it->fileContent);
        it->fileContent.clear();

        AstParams params;
        params.setTextDocument(TextDocumentIdentifier(it.key()));
        AstRequest request(params);
        request.setResponseCallback([this, key, loc = it.key(), request]
                                    (AstRequest::Response response) {
            qCDebug(clangdLog) << "AST response for" << loc.toFilePath();
            const auto refData = runningFindUsages.find(key);
            if (refData == runningFindUsages.end())
                return;
            if (!refData->search || refData->canceled)
                return;
            ReferencesFileData &data = refData->fileData[loc];
            const auto result = response.result();
            if (result)
                data.ast = *result;
            refData->pendingAstRequests.removeOne(request.id());
            qCDebug(clangdLog) << refData->pendingAstRequests.size()
                               << "AST requests still pending";
            addSearchResultsForFile(*refData, loc.toFilePath(), data);
            refData->fileData.remove(loc);
            if (refData->pendingAstRequests.isEmpty()) {
                qDebug(clangdLog) << "retrieved all ASTs";
                finishSearch(*refData, false);
            }
        });
        qCDebug(clangdLog) << "requesting AST for" << it.key().toFilePath();
        refData->pendingAstRequests << request.id();
        q->sendContent(request);

        if (extraOpen)
            q->closeExtraFile(it.key().toFilePath());
    }
}

void ClangdClient::Private::handleRenameRequest(const SearchResult *search,
                                                const ReplacementData &replacementData,
                                                const QString &newSymbolName,
                                                const QList<SearchResultItem> &checkedItems,
                                                bool preserveCase)
{
    const QStringList fileNames = TextEditor::BaseFileFind::replaceAll(newSymbolName, checkedItems,
                                                                       preserveCase);
    if (!fileNames.isEmpty())
        SearchResultWindow::instance()->hide();

    const auto renameFilesCheckBox = qobject_cast<QCheckBox *>(search->additionalReplaceWidget());
    QTC_ASSERT(renameFilesCheckBox, return);
    if (!renameFilesCheckBox->isChecked())
        return;

    QVector<Node *> fileNodes;
    for (const Utils::FilePath &file : replacementData.fileRenameCandidates) {
        Node * const node = ProjectTree::nodeForFile(file);
        if (node)
            fileNodes << node;
    }
    if (!fileNodes.isEmpty())
        CppTools::renameFilesForSymbol(replacementData.oldSymbolName, newSymbolName, fileNodes);
}

void ClangdClient::Private::addSearchResultsForFile(ReferencesData &refData,
        const Utils::FilePath &file,
        const ReferencesFileData &fileData)
{
    QList<SearchResultItem> items;
    qCDebug(clangdLog) << file << "has valid AST:" << fileData.ast.isValid();
    for (const auto &rangeWithText : fileData.rangesAndLineText) {
        const Range &range = rangeWithText.first;
        const Usage::Type usageType = fileData.ast.isValid()
                ? getUsageType(getAstPath(fileData.ast, qAsConst(range)))
                : Usage::Type::Other;
        SearchResultItem item;
        item.setUserData(int(usageType));
        item.setStyle(CppTools::colorStyleForUsageType(usageType));
        item.setFilePath(file);
        item.setMainRange(SymbolSupport::convertRange(range));
        item.setUseTextEditorFont(true);
        item.setLineText(rangeWithText.second);
        if (refData.search->supportsReplace()) {
            const bool fileInSession = SessionManager::projectForFile(file);
            item.setSelectForReplacement(fileInSession);
            if (fileInSession && file.toFileInfo().baseName().compare(
                        refData.replacementData->oldSymbolName,
                        Qt::CaseInsensitive) == 0) {
                refData.replacementData->fileRenameCandidates << file; // TODO: We want to do this only for types. Use SymbolInformation once we have it.
            }
        }
        items << item;
    }
    if (isTesting)
        emit q->foundReferences(items);
    else
        refData.search->addResults(items, SearchResult::AddOrdered);
}

void ClangdClient::Private::reportAllSearchResultsAndFinish(ReferencesData &refData)
{
    for (auto it = refData.fileData.begin(); it != refData.fileData.end(); ++it)
        addSearchResultsForFile(refData, it.key().toFilePath(), it.value());
    finishSearch(refData, refData.canceled);
}

void ClangdClient::Private::finishSearch(const ReferencesData &refData, bool canceled)
{
    if (isTesting) {
        emit q->findUsagesDone();
    } else if (refData.search) {
        refData.search->finishSearch(canceled);
        refData.search->disconnect(q);
        if (refData.replacementData) {
            const auto renameCheckBox = qobject_cast<QCheckBox *>(
                        refData.search->additionalReplaceWidget());
            QTC_CHECK(renameCheckBox);
            const QSet<Utils::FilePath> files = refData.replacementData->fileRenameCandidates;
            renameCheckBox->setText(tr("Re&name %n files", nullptr, files.size()));
            const QStringList filesForUser = Utils::transform<QStringList>(files,
                        [](const Utils::FilePath &fp) { return fp.toUserOutput(); });
            renameCheckBox->setToolTip(tr("Files:\n%1").arg(filesForUser.join('\n')));
            renameCheckBox->setVisible(true);
            refData.search->setUserData(QVariant::fromValue(*refData.replacementData));
        }
    }
    runningFindUsages.remove(refData.key);
}

} // namespace Internal
} // namespace ClangCodeModel

Q_DECLARE_METATYPE(ClangCodeModel::Internal::ReplacementData)
