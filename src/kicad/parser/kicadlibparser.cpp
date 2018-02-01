/**
 ** This file is part of the uConfig project.
 ** Copyright 2018 Robotips sebastien.caux@robotips.fr.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/

#include "kicadlibparser.h"

#include <QDateTime>
#include <QFileInfo>

KicadLibParser::KicadLibParser()
{
}

Lib *KicadLibParser::loadLib(const QString &fileName, Lib *lib)
{
    bool mylib = false;
    if (!lib)
    {
        mylib = true;
        lib = new Lib();
    }

    QFile input(fileName);
    QFileInfo info(input);
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (mylib)
            delete lib;
        return Q_NULLPTR;
    }
    _stream.setDevice(&input);

    _stream.readLine();
    lib->clear();

    Component *component;
    do
    {
        component = readComponent();
        if (component)
            lib->addComponent(component);
        else
            _stream.readLine();
    } while (!_stream.atEnd());
    lib->setName(info.baseName());

    _stream.setDevice(Q_NULLPTR);
    return lib;
}

bool KicadLibParser::saveLib(const QString &fileName, Lib *lib)
{
    QFile output(fileName);
    QFileInfo info(output);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    _stream.setDevice(&output);
    writeLib(lib);
    lib->setName(info.baseName());

    _stream.setDevice(Q_NULLPTR);
    return true;
}

/**
 * @brief Serialise the lib in Kicad format
 * @param lib lib to serialise
 */
void KicadLibParser::writeLib(Lib *lib)
{
    _stream << "EESchema-LIBRARY Version 2.3  Date: "
            << QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss")
            << '\n';
    _stream << "#encoding utf-8" << '\n';
    _stream << "#created with uConfig by Sebastien CAUX (sebcaux)" << '\n';
    _stream << "#https://github.com/Robotips/uConfig" << '\n';

    // components
    foreach (Component *component, lib->components())
    {
        writeComponent(component);
        _stream << '\n';
    }

    // footer
    _stream << "#" << '\n';
    _stream << "#End Library";
}

void KicadLibParser::writeComponent(Component *component)
{
    // http://en.wikibooks.org/wiki/Kicad/file_formats#Description_of_a_component_2

    // comments
    _stream << "#" << '\n' << "# " << component->name() << '\n' << "#" << '\n';

    // def
    _stream << "DEF " << component->name() << " " << component->prefix() << " 0 40 ";
    _stream << (component->showPadName() ? "Y " : "N ");
    _stream << (component->showPinName() ? "Y " : "N ");
    _stream << "1 F N" << '\n';

    // F0
    _stream << "F0 \"" << component->prefix() << "\" "
            << component->rect().right()-50 << " "
            << -component->rect().bottom()-50
            << " 50 H V C CNN" << '\n';

    // F1
    _stream << "F1 \"" << component->name() << "\" 0 0 50 H V C CNN" << '\n';

    // F2
    _stream << "F2 \"~\" 0 0 50 H I C CNN" << '\n';

    // F3
    _stream << "F3 \"~\" 0 0 50 H I C CNN" << '\n';

    // footprints
    if (!component->footPrints().isEmpty())
    {
        _stream << "$FPLIST" << '\n';
        foreach (const QString &footPrint, component->footPrints())
        {
            _stream << " " << footPrint << '\n';
        }
        _stream << "$ENDFPLIST" << '\n';
    }

    // alias
    if (!component->aliases().isEmpty())
        _stream << "ALIAS " << component->aliases().join(" ") << '\n';

    _stream << "DRAW" << '\n';
    // pins
    foreach (Pin *pin, component->pins())
    {
        writePin(pin);
        _stream << '\n';
    }

    // rect
    if (component->rect().isValid())
    {
        _stream << "S "
                << component->rect().left() << " "
                << component->rect().top() << " "
                << component->rect().right() << " "
                << component->rect().bottom() << " "
                << "0 1 10 f" << '\n';
    }

    // end
    _stream << "ENDDRAW" << '\n';
    _stream << "ENDDEF";
}

void KicadLibParser::writePin(Pin *pin)
{
    // http://en.wikibooks.org/wiki/Kicad/file_formats#X_record_.28Pin.29

    // X PIN_NAME PAD_NAME X_POS Y_POS LINE_WIDTH DIRECTION NAME_TEXT_SIZE
    // LABEL_TEXT_SIZE LAYER ?1? ELECTRICAL_TYPE
    _stream << "X ";
    if (pin->name().isEmpty())
        _stream << "~";
    else
        _stream << pin->name();
    _stream << " "
           << pin->padName() << " "                            // pad name
           << pin->pos().x() << " " << -pin->pos().y() << " "  // x y position
           << pin->length() << " "                             // lenght
           << pin->directionString() << " "                    // pin direction (up/down/left/right)
           << "50" << " "                                      // name text size
           << "50" << " "                                      // pad name text size
           << pin->layer() << " "
           << "1" << " "
           << pin->electricalTypeString();
    if (pin->pinType() != Pin::Normal)
        _stream << " " << pin->pinTypeString();
}

Component *KicadLibParser::readComponent()
{
    Component *component = new Component();

    QString dummy;
    bool draw = false;
    do
    {
        QString start;
        _stream >> start;
        if (start.at(0) == '#') // comment
        {
            _stream.readLine();
        }
        else if (start == "DEF")
        {
            QString name;
            _stream >> name;
            component->setName(name);

            QString prefix;
            _stream >> prefix;
            component->setPrefix(prefix);

            _stream >> dummy;
            _stream >> dummy; // text offset TODO

            QString option;
            _stream >> option;
            component->setShowPadName(option == "Y");
            _stream >> option;
            component->setShowPinName(option == "Y");

            _stream.readLine();
        }
        else if (start.at(0) == 'F')
        {
            _stream.readLine();
        }
        else if (start =="$FPLIST")
        {
            QString footprint;
            while (!_stream.atEnd())
            {
                _stream >> footprint;
                if (footprint == "$ENDFPLIST")
                    break;
                component->addFootPrint(footprint);
            }
        }
        else if (start.startsWith("DRAW"))
        {
            draw = true;
            _stream.readLine();
        }
        else if (start.startsWith("ALIAS"))
        {
            QString aliases = _stream.readLine();
            component->addAlias(aliases.split(' ', QString::SkipEmptyParts));
        }
        else if (start.startsWith("ENDDRAW"))
        {
            draw = false;
            _stream.readLine();
        }
        else if (start.startsWith("ENDDEF"))
        {
            draw = false;
            _stream.readLine();
            return component;
        }
        else if (draw)
        {
            if (start.at(0) == 'X')
            {
                Pin *pin = readPin();
                if (pin)
                    component->addPin(pin);
                else
                    _stream.readLine();
            }
            else if (start.startsWith("S"))
            {
                QRect rect;
                int n;
                _stream >> n;
                rect.setX(n);
                _stream >> n;
                rect.setY(-n);
                _stream >> n;
                rect.setRight(n);
                _stream >> n;
                rect.setBottom(-n);
                component->setRect(rect.normalized());
            }
        }
    } while (!_stream.atEnd());

    return component;
}

Pin *KicadLibParser::readPin()
{
    Pin *pin = new Pin();

    // name
    QString name;
    _stream >> name;
    if (name == "~")
        name = "";
    if (_stream.status() != QTextStream::Ok)
    {
        delete pin;
        return Q_NULLPTR;
    }
    pin->setName(name);

    // name
    QString padName;
    _stream >> padName;
    if (_stream.status() != QTextStream::Ok)
    {
        delete pin;
        return Q_NULLPTR;
    }
    pin->setPadName(padName);

    // position
    int x, y;
    _stream >> x >> y;
    if (_stream.status() != QTextStream::Ok)
    {
        delete pin;
        return Q_NULLPTR;
    }
    pin->setPos(x, -y);

    // lenght
    int lenght;
    _stream >> lenght;
    if (_stream.status() != QTextStream::Ok)
    {
        delete pin;
        return Q_NULLPTR;
    }
    pin->setLength(lenght);

    // orientation
    char direction;
    _stream.skipWhiteSpace();
    _stream >> direction;
    pin->setDirection(direction);

    // two ignored fields
    QString dummy;
    _stream >> dummy;
    _stream >> dummy;

    // layer
    int layer;
    _stream >> layer;
    if (_stream.status() != QTextStream::Ok)
    {
        delete pin;
        return Q_NULLPTR;
    }
    pin->setLayer(layer);

    _stream.skipWhiteSpace();
    _stream >> dummy;

    // elec type
    char elec_type;
    _stream.skipWhiteSpace();
    _stream >> elec_type;
    pin->setElectricalType(elec_type);

    // pin type
    QString pinType = _stream.readLine();
    pin->setPinType(pinType.trimmed());

    return pin;
}