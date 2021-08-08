#include "csvmodelwriter.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QTextStream>

CSVModelWriter::CSVModelWriter(const QString &filename, QObject *parent) :
    QObject(parent),
    filename(filename), model(0)
{
}

void CSVModelWriter::setModel(const QAbstractItemModel *model)
{
    this->model = model;
}

void CSVModelWriter::addColumn(const QString &title, int column, int role)
{
    Column col;
    col.title = title;
    col.column = column;
    col.role = role;

    columns.append(col);
}

static void writeValue(QTextStream &f, const QString &value)
{
    QString escaped = value;
    escaped.replace('"', "\"\"");
    f << "\"" << escaped << "\"";
}

static void writeSep(QTextStream &f)
{