#ifndef TPS_PARSER_H
#define TPS_PARSER_H

#include <QtCore>
#include <QDomElement>

class TPSParser {
public:
    static QVariant parse(QIODevice *device);
private:
	static QVariant parseElement(const QDomElement &e);
	static QVariantList parseArrayElement(const QDomElement& node);
	static QVariantMap parseDictElement(const QDomElement& element);
};

#endif
