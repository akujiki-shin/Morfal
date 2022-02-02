#pragma once

#include <QString>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QIODevice>

namespace SaveUtils
{
    template<typename T>
    void Save(QDataStream& stream, const QString& /*dataName*/, const T& data)
    {
        stream << data;
    }

    template<typename T>
    void SaveAsByte(QDataStream& stream, const QString& /*dataName*/, const T& data)
    {
        stream << data;
    }

    template<typename T>
    void Load(QDataStream& stream, const QString& /*dataName*/, T& data)
    {
        stream >> data;
    }

    template<typename T>
    void LoadFromByte(QDataStream& stream, const QString& /*dataName*/, T& data)
    {
        stream >> data;
    }

    QDataStream* OpenSaveScope(QDataStream& stream);
    QJsonObject* OpenSaveScope(QJsonObject& jsonObject);
    void CloseSaveScope(QDataStream* stream);
    void CloseSaveScope(QJsonObject* jsonObject);

    template<typename T>
    struct ParentSaveScope;

    template<>
    struct ParentSaveScope<QDataStream>
    {
        using type = char;
    };

    template<>
    struct ParentSaveScope<QJsonObject>
    {
        using type = QJsonArray;
    };

    template<typename T>
    struct LoadScopeObject;

    template<>
    struct LoadScopeObject<QDataStream>
    {
        using type = char;
    };

    template<>
    struct LoadScopeObject<QJsonObject>
    {
        using type = QJsonObject;
    };

    ParentSaveScope<QDataStream>::type LoadParent(QDataStream& stream, const QString& name);
    QJsonArray LoadParent(QJsonObject& stream, const QString& name);

    QDataStream& AsStream(QDataStream& stream, LoadScopeObject<QDataStream>::type& dummy, ParentSaveScope<QDataStream>::type& parent, int index);
    QJsonObject& AsStream(QJsonObject& stream, QJsonObject& scope, QJsonArray& parent, int index);

    void SaveParent(QDataStream& parentScope, const QString& dataName, ParentSaveScope<QDataStream>::type& scopeTsSave);
    void SaveParent(QJsonObject& jsonObject, const QString& dataName, QJsonArray& data);
    void SaveScope(ParentSaveScope<QDataStream>::type& parentScope, QDataStream& scopeToSave);
    void SaveScope(QJsonArray& parentScope, QJsonObject& scopeToSave);

    template<typename T>
    void Save(QJsonObject& jsonObject, const QString& dataName, const T& data)
    {
        jsonObject[dataName] = data;
    }

    template<>
    void Save(QJsonObject& jsonObject, const QString& dataName, const int& data);

    template<typename T>
    void SaveAsByte(QJsonObject& jsonObject, const QString& dataName, const T& data)
    {
        QByteArray asByte;
        QDataStream stream(&asByte, QIODevice::WriteOnly);
        stream << data;

        QByteArray asByte64 = asByte.toBase64();
        QString asString(asByte64);
        jsonObject[dataName] = asString;
    }

    template<typename T>
    void LoadFromByte(QJsonObject& jsonObject, const QString& dataName, T& data)
    {
        QByteArray asByte64;
        asByte64.append(jsonObject[dataName].toString().toStdString());
        QByteArray asByte = QByteArray::fromBase64(asByte64);

        QDataStream stream(&asByte, QIODevice::ReadOnly);
        stream >> data;
    }

    template<typename T>
    void Load(QJsonObject& jsonObject, const QString& dataName, T& data)
    {
        data = jsonObject[dataName].toString();
    }

    template<>
    void Load(QJsonObject& jsonObject, const QString& dataName, bool& data);

    template<>
    void Load(QJsonObject& jsonObject, const QString& dataName, int& data);

    template<>
    void Load(QJsonObject& jsonObject, const QString& dataName, double& data);

    template<>
    void Load(QJsonObject& jsonObject, const QString& dataName, int& data);
}
