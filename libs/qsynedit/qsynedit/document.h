/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SYNEDITSTRINGLIST_H
#define SYNEDITSTRINGLIST_H

#include <QStringList>
#include "syntaxer/syntaxer.h"
#include <QFontMetrics>
#include <QMutex>
#include <QVector>
#include <memory>
#include <QFile>
#include "miscprocs.h"
#include "types.h"
#include "qt_utils/utils.h"

namespace QSynedit {

QList<int> calcGlyphPositions(const QString &text);

class Document;

/**
 * @brief The DocumentLine class
 *
 * Store one line of the document.
 * The linebreak is not included.
 *
 * When the line is displayed on the screen, each mark is called a glyph.
 * In unicode, a glyph may be represented by more than one code points (chars).
 * DocumentLine provides utility methods to retrieve the chars corresponding to one glyph,
 * and other functions.
 *
 * Most of the member methods are not thread safe. So they are declared as private
 * to prevent ill-usage. It shoulde only be used by the document internally.
 */
class DocumentLine {
public:
    explicit DocumentLine();
    DocumentLine(const DocumentLine&)=delete;
    DocumentLine& operator=(const DocumentLine&)=delete;

private:
    /**
     * @brief get total count of the glyphs in the line text
     *
     * The char count of the line text may not be the same as the glyphs count
     *
     * @return the glyphs count
     */
    int glyphsCount() const {
        return mGlyphPositions.length();
    }

    /**
     * @brief get list of start index of the glyphs in the line text
     * @return start positions of the glyph.
     */
    const QList<int>& glyphPositions() const {
        return mGlyphPositions;
    }

    /**
     * @brief get list of start column of the glyphs in the line text
     * @return start positions of the glyph.
     */
    const QList<int>& glyphColumns() const {
        return mGlyphColumns;
    }

    /**
     * @brief get start index of the chars representing the specified glyph.
     * @param i index of the glyph of the line (starting from 0)
     * @return
     */
    int glyphStart(int i) const {
        Q_ASSERT(i>=0 && i<mGlyphPositions.length());
        return mGlyphPositions[i];
    }

    /**
     * @brief get end index of the chars representing the specified glyph.
     * @param i index of the glyph of the line (starting from 0)
     * @return
     */
    int glyphEnd(int i) const;

    /**
     * @brief get the chars representing the specified glyph.
     * @param i index of the glyph of the line (starting from 0)
     * @return the chars representing the specified glyph
     */
    QString getGlyph(int i) const;

    /**
     * @brief get start column of the specified glyph.
     * @param i index of the glyph of the line (starting from 0)
     * @return
     */
    int getGlyphStartColumn(int i) const {
        Q_ASSERT(mColumns>=0);
        Q_ASSERT(i>=0 && i<mGlyphColumns.length());
        return mGlyphColumns[i];
    }

    /**
     * @brief get end column of the specified glyph.
     * @param i index of the glyph of the line (starting from 0)
     * @return
     */
    int getGlyphEndColumn(int i) const;

    /**
     * @brief get the line text
     * @return the line text
     */
    const QString& lineText() const { return mLineText; }

    /**
     * @brief get the width (in columns) of the line text
     * @return the width (in columns)
     */
    int columns() const { return mColumns; }

    /**
     * @brief get the state of the syntax highlighter after this line is parsed
     * @return
     */
    const SyntaxState& syntaxState() const { return mSyntaxState; }
    /**
     * @brief set the state of the syntax highlighter after this line is parsed
     * @param newSyntaxState
     */
    void setSyntaxState(const SyntaxState &newSyntaxState) { mSyntaxState = newSyntaxState; }

    void setLineText(const QString &newLineText);
    void setColumns(int cols, QList<int> glyphCols) { mColumns = cols; mGlyphColumns = glyphCols; }
    void invalidateColumns() { mColumns = -1; mGlyphColumns.clear(); }
private:
    QString mLineText; /* the unicode code points of the text */
    /**
     * @brief Start positions of glyphs in mLineText
     *
     * A glyph may be defined by more than one code points.
     * Each lement of mGlyphPositions (position) is the start index
     *  of the code points in the mLineText.
     */
    QList<int> mGlyphPositions;
    /**
     * @brief start columns of the glyphs
     *
     * A glyph may occupy more than one columns in the screen.
     * Each elements of mGlyphColumns is the columns occupied by the glyph.
     * The width of a glyph is affected by the font used to display,
     * so it must be recalculated each time the font is changed.
     */
    QList<int> mGlyphColumns;
    /**
     * @brief state of the syntax highlighter after this line is parsed
     *
     * QSynedit use this state to speed up syntax highlight parsing.
     * Which is also used in auto-indent calculating and other functions.
     */
    SyntaxState mSyntaxState;
    /**
     * @brief total width (in columns) of the line text
     *
     * The width of glyphs is affected by the font used to display,
     * so it must be recalculated each time the font is changed.
     */
    int mColumns;

    friend class Document;
};

typedef std::shared_ptr<DocumentLine> PDocumentLine;

typedef QVector<PDocumentLine> DocumentLines;

typedef std::shared_ptr<DocumentLines> PDocumentLines;

typedef std::shared_ptr<Document> PDocument;

class BinaryFileError : public FileError {
public:
    explicit BinaryFileError (const QString& reason);
};

/**
 * @brief The Document class
 *
 * Represents a document, which contains many lines.
 *
 */
class Document : public QObject
{  
    Q_OBJECT
public:
    explicit Document(const QFont& font, const QFont& nonAsciiFont, QObject* parent=nullptr);
    Document(const Document&)=delete;
    Document& operator=(const Document&)=delete;

    /**
     * @brief get nesting level of parenthesis at the end of the specified line
     *
     * It's thread safe.
     *
     * @param line line index (starts from 0)
     * @return
     */
    int parenthesisLevel(int line);

    /**
     * @brief get nesting level of brackets at the end of the specified line
     *
     * It's thread safe
     *
     * @param line line index (starts from 0)
     * @return
     */
    int bracketLevel(int line);

    /**
     * @brief get nesting level of braces at the end of the specified line
     *
     * It's thread safe
     *
     * @param line line index (starts from 0)
     * @return
     */
    int braceLevel(int line);

    /**
     * @brief get width (in columns) of the specified line
     *
     * It's thread safe
     *
     * @param line line index (starts frome 0)
     * @return
     */
    int lineColumns(int line);

    /**
     * @brief get width (in columns) of the specified text / line
     *
     * It's thread safe.
     * If the new text is the same as the line text, it just
     * returns the line width pre-calculated.
     * If the new text is not the same as the line text, it
     * calculates the width of the new text and return.
     *
     * @param line line index (starts frome 0)
     * @param newText the new text
     * @return
     */
    int lineColumns(int line, const QString &newText);

    /**
     * @brief get block (indent) level of the specified line
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    int blockLevel(int line);

    /**
     * @brief get count of new blocks (indent) started on the specified line
     * @param line line index (starts frome 0)
     * @return
     */
    int blockStarted(int line);

    /**
     * @brief get count of blocks (indent) ended on the specified line
     * @param line line index (starts frome 0)
     * @return
     */
    int blockEnded(int line);

    /**
     * @brief get index of the longest line (has the max width)
     *
     * It's thread safe.
     *
     * @return
     */
    int longestLineColumns();

    /**
     * @brief get line break of the current document
     *
     * @return
     */
    QString lineBreak() const;

    /**
     * @brief get state of the syntax highlighter after parsing the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    SyntaxState getSyntaxState(int line);

    /**
     * @brief set state of the syntax highlighter after parsing the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @param state the new state
     */
    void setSyntaxState(int line, const SyntaxState& state);

    /**
     * @brief get line text of the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    QString getLine(int line);

    /**
     * @brief get count of the glyphs on the specified line.
     *
     * It's thread safe.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    int getLineGlyphsCount(int line);

    /**
     * @brief get position list of the glyphs on the specified line.
     *
     * It's thread safe.
     * Each element of the list is the index of the starting char in the line text.
     *
     * @param line line index (starts frome 0)
     * @return
     */
    QList<int> getGlyphPositions(int index);

    /**
     * @brief get count of lines in the document
     *
     * It's thread safe.
     *
     * @return
     */
    int count();

    /**
     * @brief get all the text in the document.
     *
     * Lines are concatenated by line breaks (by lineBreak()).
     * It's thread safe.
     *
     * @return
     */
    QString text();

    /**
     * @brief set the text of the document
     *
     * It's thread safe.
     *
     * @param text
     */
    void setText(const QString& text);


    /**
     * @brief set the text of the document
     *
     * It's thread safe.
     *
     * @param text
     */
    void setContents(const QStringList& text);

    /**
     * @brief get all the lines in the document.
     *
     * It's thread safe.
     *
     * @return
     */
    QStringList contents();

    void putLine(int index, const QString& s, bool notify=true);

    void beginUpdate();
    void endUpdate();

    int addLine(const QString& s);
    void addLines(const QStringList& strings);

    int getTextLength();
    void clear();
    void deleteAt(int index);
    void deleteLines(int index, int numLines);
    void exchange(int index1, int index2);
    void insertLine(int index, const QString& s);
    void insertLines(int index, int numLines);

    void loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding);
    void saveToFile(QFile& file, const QByteArray& encoding,
                    const QByteArray& defaultEncoding, QByteArray& realEncoding);

    /**
     * @brief calculate display width (in columns) of a string
     *
     * The string may contains tab char, whose width depends on the tab size and it's position
     *
     * @param str the string to be displayed
     * @param colsBefore columns before the string
     * @return width of the string, don't including colsBefore
     */
    int stringColumns(const QString &str, int colsBefore) const;
    int glyphStart(int line, int glyphIdx);
    int glyphEnd(int line, int glyphIdx);
    int glyphStartColumn(int line, int glyphIdx);
    int glyphEndColumn(int line, int glyphIdx);
    int charToGlyphIndex(int line, int charPos);

    int charToColumn(int line, int charPos);
    int columnToChar(int line, int column);
    int charToColumn(int line, const QString newStr, int charPos);
    int columnToChar(int line, const QString newStr, int column);
    int columnToGlyphIndex(int line, int column);



    int charToColumn(const QString& str, int charPos);
    int charToColumn(const QString& lineText, const QList<int> &glyphPositions, int charPos);
    int columnToChar(const QString& lineText, int column);
    int columnToChar(const QString& lineText, const QList<int> &glyphPositions, int column);


    bool getAppendNewLineAtEOF();
    void setAppendNewLineAtEOF(bool appendNewLineAtEOF);

    NewlineType getNewlineType();
    void setNewlineType(const NewlineType &fileEndingType);

    bool empty();

    int tabWidth() const {
        return mTabWidth;
    }
    void setTabWidth(int newTabWidth);

    const QFontMetrics &fontMetrics() const;
    void setFontMetrics(const QFont &newFont, const QFont& newNonAsciiFont);

public slots:
    void invalidateAllLineColumns();

signals:
    void changed();
    void changing();
    void cleared();
    void deleted(int startLine, int count);
    void inserted(int startLine, int count);
    void putted(int startLine, int count);
protected:
    QString getTextStr() const;
    void setUpdateState(bool Updating);
    void insertItem(int line, const QString& s);
    void addItem(const QString& s);
    void putTextStr(const QString& text);
    void internalClear();
private:
    QList<int> calcGlyphColumns(const QString& lineText, const QList<int> &glyphPositions, int colsBefore, int &totalColumns) const;
    bool tryLoadFileByEncoding(QByteArray encodingName, QFile& file);
    void loadUTF16BOMFile(QFile& file);
    void loadUTF32BOMFile(QFile& file);
    void saveUTF16File(QFile& file, QTextCodec* codec);
    void saveUTF32File(QFile& file, QTextCodec* codec);

private:
    DocumentLines mLines;

    //SynEdit* mEdit;

    QFontMetrics mFontMetrics;
    QFontMetrics mNonAsciiFontMetrics;
    int mTabWidth;
    int mCharWidth;
    //int mCount;
    //int mCapacity;
    NewlineType mNewlineType;
    bool mAppendNewLineAtEOF;
    int mIndexOfLongestLine;
    int mUpdateCount;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QRecursiveMutex mMutex;
#else
    QMutex mMutex;
#endif

    int calculateLineColumns(int Index);
};

enum class ChangeReason {
    Insert,
    Delete,
    Caret, //just restore the Caret, allowing better Undo behavior
    Selection, //restore Selection
    GroupBreak,
    LeftTop,
    LineBreak,
    MoveSelectionUp,
    MoveSelectionDown,
    ReplaceLine,
    Nothing // undo list empty
  };

class UndoItem {
private:
    ChangeReason mChangeReason;
    SelectionMode mChangeSelMode;
    BufferCoord mChangeStartPos;
    BufferCoord mChangeEndPos;
    QStringList mChangeText;
    size_t mChangeNumber;
    unsigned int mMemoryUsage;
public:
    UndoItem(ChangeReason reason,
        SelectionMode selMode,
        BufferCoord startPos,
        BufferCoord endPos,
        const QStringList& text,
        int number);

    ChangeReason changeReason() const;
    SelectionMode changeSelMode() const;
    BufferCoord changeStartPos() const;
    BufferCoord changeEndPos() const;
    QStringList changeText() const;
    size_t changeNumber() const;
    unsigned int memoryUsage() const;
};

using PUndoItem = std::shared_ptr<UndoItem>;

class UndoList : public QObject {
    Q_OBJECT
public:
    explicit UndoList();

    void addChange(ChangeReason reason, const BufferCoord& start, const BufferCoord& end,
      const QStringList& changeText, SelectionMode selMode);

    void restoreChange(ChangeReason reason, const BufferCoord& start, const BufferCoord& end,
                       const QStringList& changeText, SelectionMode selMode, size_t changeNumber);

    void restoreChange(PUndoItem item);

    void addGroupBreak();
    void beginBlock();
    void endBlock();

    void clear();
    ChangeReason lastChangeReason();
    bool isEmpty();
    PUndoItem peekItem();
    PUndoItem popItem();

    bool canUndo();
    int itemCount();

    int maxUndoActions() const;
    void setMaxUndoActions(int maxUndoActions);
    bool initialState();
    void setInitialState();

    bool insideRedo() const;
    void setInsideRedo(bool insideRedo);

    bool fullUndoImposible() const;

    int maxMemoryUsage() const;
    void setMaxMemoryUsage(int newMaxMemoryUsage);

signals:
    void addedUndo();
protected:
    void ensureMaxEntries();
    bool inBlock();
    unsigned int getNextChangeNumber();
    void addMemoryUsage(PUndoItem item);
    void reduceMemoryUsage(PUndoItem item);
protected:
    size_t mBlockChangeNumber;
    int mBlockLock;
    int mBlockCount; // count of action blocks;
    int mMemoryUsage;
    size_t mLastPoppedItemChangeNumber;
    size_t mLastRestoredItemChangeNumber;
    bool mFullUndoImposible;
    QVector<PUndoItem> mItems;
    int mMaxUndoActions;
    int mMaxMemoryUsage;
    unsigned int mNextChangeNumber;
    unsigned int mInitialChangeNumber;
    bool mInsideRedo;
};

class RedoList : public QObject {
    Q_OBJECT
public:
    explicit RedoList();

    void addRedo(ChangeReason AReason, const BufferCoord& AStart, const BufferCoord& AEnd,
                 const QStringList& ChangeText, SelectionMode SelMode, size_t changeNumber);
    void addRedo(PUndoItem item);

    void clear();
    ChangeReason lastChangeReason();
    bool isEmpty();
    PUndoItem peekItem();
    PUndoItem popItem();

    bool canRedo();
    int itemCount();

protected:
    QVector<PUndoItem> mItems;
};


using PUndoList = std::shared_ptr<UndoList>;
using PRedoList = std::shared_ptr<RedoList>;

}

#endif // SYNEDITSTRINGLIST_H
