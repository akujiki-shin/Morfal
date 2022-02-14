#include "utils.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QCoreApplication>
#include <QStringRef>

#include <QTableWidget>

#include <string>

#include "minimalscopeprofiler.h"

namespace Utils
{
    bool TryFindDir(const QString& dirName, QFileInfo& fileInfo)
    {
        QDirIterator it(QCoreApplication::applicationDirPath(), QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            if (it.fileName() == dirName)
            {
                fileInfo = it.fileInfo();
                return true;
            }

            it.next();
        }

        return false;
    }

    bool TryFindDirFromPath(const QString& path, const QString& dirName, QFileInfo& fileInfo)
    {
        QDirIterator it(path, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            it.next();

            if (it.fileName() == dirName)
            {
                fileInfo = it.fileInfo();
                return true;
            }
        }

        return false;
    }

    bool TryFindDirPath(const QString& dirName, QString& path)
    {
        QFileInfo dirInfo;
        if (TryFindDir(dirName, dirInfo))
        {
            path = dirInfo.filePath();
            return true;
        }

        return false;
    }

    QFileInfo GetFileInfoFromPath(const QString& path)
    {
        QDirIterator it(path, QDirIterator::NoIteratorFlags);
        it.next();
        return it.fileInfo();
    }

    void GetDirectSubDirList(const QDir& dir, std::vector<QFileInfo>& subDir)
    {
        QDirIterator it(dir.path(), QDirIterator::NoIteratorFlags);
        while (it.hasNext())
        {
            it.next();
            if (it.fileName() != "." && it.fileName() != "..")
            {
                subDir.push_back(it.fileInfo());
            }
        }
    }

    QString GetLocalDataPath(const QDirIterator& file, const QString& dataPath)
    {
        int fullSize = file.filePath().size();
        if (file.fileInfo().isDir())
        {
             fullSize += file.fileName().size();
        }

        int localPathSize = fullSize - dataPath.size() - file.fileName().size();
        return file.filePath().toStdString().substr(dataPath.size() + 1, localPathSize - 1).c_str();
    }

    bool TryGetFirstFilePathIn(const QString& directoryName, const QString& extension, QString& filePath)
    {
        QString dataPath;
        if (TryFindDirPath(directoryName, dataPath))
        {
            QDirIterator it(dataPath, QDirIterator::Subdirectories);
            bool hasNext = true;

            while(hasNext)
            {
                if (FileMatchesExtension(it, extension))
                {
                    filePath = dataPath + "/" + it.fileName();
                    return true;
                }

                hasNext = it.hasNext();
                it.next();
            }
        }

        return false;
    }

    bool IsFileNameValid(const QString& name)
    {
        return (name != "" && name != "." && name != "..");
    }

    bool FileMatchesExtension(const QString& name, const QString& extension)
    {
        int extensionSize = extension.size();
        int nameSize = name.size();

        const std::string& stdName = name.toStdString();
        return (nameSize > extensionSize && stdName.substr(nameSize - extensionSize, nameSize) == extension.toStdString());
    }

    bool FileMatchesExtension(const QDirIterator& dirIterator, const QString& extension)
    {
        const QString& fileName = dirIterator.fileName();

        return (dirIterator.fileInfo().isFile() && IsFileNameValid(fileName) && FileMatchesExtension(fileName, extension));
    }

    QString RemoveWhiteSpaces(const QString& qstring)
    {
        std::string string = qstring.toStdString();
        string.erase(std::remove_if(string.begin(), string.end(), ::isspace), string.end());
        QString result(string.c_str());
        return result;
    }

    QString GetSubStringBetweenSeparators(const QString& text, const QChar& start, const QChar& stop, int offset)
    {
        QString result;

        if (offset < text.length())
        {
            int startPosition = -1;
            int stopPosition = -1;

            for (int i = offset; i >= 0; --i)
            {
                const QChar& currentChar = text.at(i);
                if (currentChar == start)
                {
                    startPosition = i + 1;
                    break;
                }
                else if (currentChar == stop)
                {
                    break;
                }
            }

            for (int i = offset; i < text.length(); ++i)
            {
                const QChar& currentChar = text.at(i);
                if (currentChar == stop)
                {
                    stopPosition = i;
                    break;
                }
                else if (currentChar == start)
                {
                    break;
                }
            }

            if (startPosition != -1 && stopPosition != -1)
            {
                QStringRef stringRef(&text, startPosition, stopPosition - startPosition);
                result = stringRef.toString();
            }
        }

        return result;
    }

    QString RemoveJsonExtension(const QString& name)
    {
        static std::string jsonExtension = ".json";
        static std::size_t jsonSize = jsonExtension.size();

        return name.toStdString().substr(0, name.size() - jsonSize).c_str();
    }

    QString CleanFileName(const QString& name)
    {
        std::string stdName = name.toStdString();
        std::size_t dotPosition = stdName.find_last_of(".");
        std::size_t slashPosition = stdName.find_last_of("/");

        return stdName.substr(slashPosition + 1, dotPosition - slashPosition - 1).c_str();
    }

    QString ReplaceExtensionBy(const QString& name, const QString& newExtension)
    {
        std::string stdName = name.toStdString();
        std::size_t dotPosition = stdName.find_last_of(".");

        return stdName.substr(0, dotPosition).c_str() + newExtension;
    }

    bool Contains(const QList<QListWidgetItem*>& list, const QString& name)
    {
        for (const QListWidgetItem* item : list)
        {
            if (item->data(Qt::DisplayRole).toString() == name)
            {
                return true;
            }
        }

        return false;
    }

    bool Contains(const QListWidget& list, const QString& name)
    {
       return (list.findItems(name, Qt::MatchExactly).count() > 0);
    }

    void DeleteAll(QListWidget& list)
    {
        for (int i = 0; i < list.count(); ++i)
        {
            delete list.takeItem(i);
        }

        list.clear();
    }

    void ComputeMergedJSon(const std::vector<std::vector<QString>>& elementsList, QString& result)
    {
        profile();

        std::size_t totalItemCount = 0;

        int totalStringSize = 1;

        for (const auto& elements : elementsList)
        {
            totalItemCount += elements.size();

            for (const QString& element : elements)
            {
                totalStringSize += element.size() + 1;
            }
        }

        result.clear();
        result.reserve(totalStringSize);

        const bool hasMultipleItems = (totalItemCount > 1);
        if (hasMultipleItems)
        {
            result += "[";
        }

        std::size_t processedItemCount = 0;

        for (const auto& elements : elementsList)
        {
            for (const QString& data : elements)
            {
                result += data;

                if (processedItemCount < totalItemCount - 1)
                {
                    result += ",";
                }

                processedItemCount++;
            }
        }

        if (hasMultipleItems)
        {
            result += "]";
        }
    }

    QString Verticalize(const QString& text)
    {
        QString result;
        for (int i = 0; i < text.size(); ++i)
        {
            result += QString(text.at(i)) + "\n";
        }

        return result;
    }

    void SelectViewItemByName(QListView& listView, const QString& name, bool selected)
    {
        QItemSelectionModel::SelectionFlags selectionFlag = selected ? QItemSelectionModel::Select : QItemSelectionModel::Deselect;

        QAbstractItemModel* model = listView.model();
        const int rowCount = model->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            const QModelIndex& index = model->index(i, 0);
            if (index.data().toString() == name && listView.selectionModel()->isSelected(index) != selected)
            {
                listView.selectionModel()->select(index, selectionFlag);
            }
        }
    }

    int GetSelectedRowCount(const QTableWidget& table)
    {
        int rowCount = 0;

        for (const QTableWidgetSelectionRange& range : table.selectedRanges())
        {
            rowCount += range.rowCount();
        }

        return rowCount;
    }

    void MoveRow(QTableWidget& table, int sourceRow, int destinationRow)
    {
        Q_ASSERT(destinationRow >= 0 && destinationRow < table.rowCount());
        Q_ASSERT(sourceRow >= 0 && sourceRow < table.rowCount());

        QList<QTableWidgetItem*> sourceItems = TakeRow(table, sourceRow);
        SetRow(table, destinationRow, sourceItems);
    }

    QList<QTableWidgetItem*> TakeRow(QTableWidget& table, int row)
    {
        QList<QTableWidgetItem*> rowItems;

        for (int column = 0; column < table.columnCount(); ++column)
        {
            rowItems.append(table.takeItem(row, column));
        }

        table.removeRow(row);

        return rowItems;
    }

    void SetRow(QTableWidget& table, int row, const QList<QTableWidgetItem*>& rowItems)
    {
        table.insertRow(row);

        for (int column = 0; column < table.columnCount(); ++column)
        {
            table.setItem(row, column, rowItems.at(column));
        }
    }
}
