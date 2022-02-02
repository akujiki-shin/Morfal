#pragma once

#include <QString>

class QJsonValue;
class QJsonObject;
class QJSValue;

class JSonRetriever
{
public:
    JSonRetriever(const QString& prefix, const QString& path, const QString& suffix, const QString& varName);

    QJsonValue GetJsonValue(const QJsonValue& jsonValue, const QString& fullPath, QMap<QString, QJSValue>* varMap = nullptr) const;

    void SetItemNamePath(const QString& path);
    const QString& GetItemNamePath();

    QString ToString(const QJsonObject& jsonObject) const;
    QString ToString(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap = nullptr) const;

    bool IsValid(const QJsonObject& jsonObject, QMap<QString, QJSValue>* varMap = nullptr) const;
    bool IsValid(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap = nullptr) const;

    virtual void ToObjectList(const QJsonValue& /*jsonValue*/, QList<QJsonObject>& /*objectList*/);

protected:
    const QString& GetPath() const;
    virtual QString ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const = 0;
    virtual bool IsValidInternal(const QJsonValue& /*jsonValue*/, QMap<QString, QJSValue>* varMap = nullptr) const;

private:
    QString m_Prefix;
    QString m_Path;
    QString m_Suffix;
    QString m_VarName;
    QString m_ItemNamePath;
};
