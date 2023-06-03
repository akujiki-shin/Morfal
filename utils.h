// Morfal is a Virtual Pen and Paper (VPP): a Game Master's toolbox designed for those who play Role Playing Games with pen and paper only,
// as opposed to Virtual Table Tops (VTT) which are made to handle tactical gameplay (tokens, square/hexagons, area of effect, etc...).
// Copyright (C) 2022  akujiki
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <QDirIterator>
#include <QString>
#include <QMetaEnum>
#include <vector>
#include <algorithm>
#include <functional>
#include <QRegularExpression>

class QListView;
class QListWidget;
class QListWidgetItem;
class QTableWidget;
class QTableWidgetItem;

namespace Utils
{
    bool TryFindDir(const QString& dirName, QFileInfo& fileInfo);
    bool TryFindDirFromPath(const QString& path, const QString& dirName, QFileInfo& fileInfo);
    bool TryFindDirPath(const QString& dirName, QString& path);
    void GetDirectSubDirList(const QDir& dir, std::vector<QFileInfo>& subDir);

    QFileInfo GetFileInfoFromPath(const QString& path);

    QString GetLocalDataPath(const QDirIterator& file, const QString& dataPath);

    bool IsFileNameValid(const QString& name);

    bool FileMatchesExtension(const QString& name, const QString& extension);

    bool FileMatchesExtension(const QDirIterator& dirIterator, const QString& extension);

    QString RemoveWhiteSpaces(const QString& qstring);
    QString GetSubStringBetweenSeparators(const QString& text, const QChar& start, const QChar& stop, int offset = 0);
    bool TryGetDiceExpression(const QString& sourceText, QString& result, int offset = 0);

    QString ReplaceExtensionBy(const QString& name, const QString& newExtension);
    QString RemoveJsonExtension(const QString& name);
    QString CleanFileName(const QString& name);
    void ComputeMergedJSon(const std::vector<std::vector<QString>>& elementsList, QString& result);

    bool Contains(const QList<QListWidgetItem*>& list, const QString& name);
    bool Contains(const QListWidget& list, const QString& name);

    void DeleteAll(QListWidget& list);

    QString Verticalize(const QString& text);

    void SelectViewItemByName(QListView& listView, const QString& name, bool selected);

    int GetSelectedRowCount(const QTableWidget& table);

    void MoveRow(QTableWidget& table, int sourceRow, int destinationRow);

    QList<QTableWidgetItem*> TakeRow(QTableWidget& table, int row);

    void SetRow(QTableWidget& table, int row, const QList<QTableWidgetItem*>& rowItems);

    template<typename QEnum>
    std::string QtEnumToString(const QEnum value)
    {
      return std::string(QMetaEnum::fromType<QEnum>().valueToKey((int)value));
    }

    template<typename QEnum>
    std::string QtEnumToString(int value)
    {
      return std::string(QMetaEnum::fromType<QEnum>().valueToKey(value));
    }

    template<typename QEnum>
    QEnum StringToQtEnum(const QString& key, bool& matchesEnum)
    {
      return (QEnum)QMetaEnum::fromType<QEnum>().keyToValue(key.toStdString().c_str(), &matchesEnum);
    }

    template<typename T>
    bool Contains(const std::vector<T>& vector, const T& item)
    {
        return std::find(vector.cbegin(), vector.end(), item) != vector.end();
    }

    template<typename T>
    using EraseIfPredicate = std::function<bool(const T&)>;

    template<typename T>
    inline void EraseIf(std::vector<T>& vector, const EraseIfPredicate<T>& predicate)
    {
        vector.erase(std::remove_if(vector.begin(), vector.end(), predicate), vector.end());
    }

    bool TryGetFirstFilePathIn(const QString& directoryName, const QString& extension, QString& filePath);

    template<class T>
    using ParseAction = void (T::*)(const QDirIterator&, const QString&);

    template<class T>
    void ParseFiles(const QString& directoryName, const QString& extension, T& caller, ParseAction<T> action)
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
                    const QString localPath = GetLocalDataPath(it, dataPath);
                    (caller.*action)(it, localPath);
                }

                hasNext = it.hasNext();
                it.next();
            }
        }
    }

    template<class TCaller, class TExtractor>
    using ParseActionWithExtractor = void (TCaller::*)(QDirIterator&, const QString&, TExtractor&, const QString&);

    template<class TCaller, class TExtractor>
    void ParseFilesWithExtractor(const QString& directoryName, const QString& extension, TCaller& caller, ParseActionWithExtractor<TCaller, TExtractor> action, TExtractor& extractor)
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
                    const QString localPath = GetLocalDataPath(it, dataPath);
                    const QString dummyExtension = "";

                    (caller.*action)(it, localPath, extractor, dummyExtension);
                }

                hasNext = it.hasNext();
                it.next();
            }
        }
    }

    template<class TCaller, class TExtractor>
    void ParseDirectoriesWithExtractor(const QString& directoryName, const QString& extension, TCaller& caller, ParseActionWithExtractor<TCaller, TExtractor> action, TExtractor& extractor)
    {
        QString dataPath;
        if (TryFindDirPath(directoryName, dataPath))
        {
            QDirIterator it(dataPath, QDirIterator::Subdirectories);
            bool hasNext = true;

            while(hasNext)
            {
                if (it.fileInfo().isDir() && IsFileNameValid(it.fileInfo().fileName()))
                {
                    const QString localPath = GetLocalDataPath(it, dataPath);
                    (caller.*action)(it, localPath, extractor, extension);
                }

                hasNext = it.hasNext();
                it.next();
            }
        }
    }
}
