#include <JsonWrapper.h>

JsonWrapper::Gpio::Gpio(int p, const char *n, const char *l, bool d) : name{n}, label{l}
{
    pin = p;
    enable = d;
}

JsonWrapper::Gpio::Gpio(int p, const char *n, const char *l, void (*c)(bool)) : name{n}, label{l}
{
    pin = p;
    func = c;
    enable = true;
}

void JsonWrapper::Gpio::addToJson(JsonWrapper *web, JsonArray &_array)
{
    lastState = digitalRead(pin);
    web->addGpioToJson(lastState, name, pin, enable, label, _array);
}

bool JsonWrapper::Gpio::isUpdate()
{
    return digitalRead(pin) != lastState;
}

bool JsonWrapper::Gpio::isSameName(const char *n)
{
    return strcmp(name, n) == 0;
}

bool JsonWrapper::Gpio::isSameLabel(const char *l)
{
    return strcmp(label, l) == 0;
}

void JsonWrapper::Gpio::callback(JsonObject &_doc)
{
    if (enable == true)
    {
        Serial.println("gpio callback");
        const char *n = _doc["name"];
        if (strcmp(name, n) == 0 && _doc["val"].is<bool>())
        {
            bool v = _doc["val"];
            Serial.println(v);
            Serial.println("Correct json object, call callback");
            // Serial.println(*func);
            if (func)
            {
                Serial.println("tehe");
                func(v);
            }
            else
            {
                Serial.println(":(");
                defaultFunc(v);
            }
        }
        else
        {
            Serial.println("Failed json object");
            Serial.println(_doc["val"].is<bool>());
        }
    }
}

void JsonWrapper::Gpio::defaultFunc(bool b)
{
    Serial.println(b);
    Serial.println("nothing here");
    digitalWrite(pin, b);
}



JsonWrapper::Function::Function(const char *n, void (*c)(const char *p)) : name{n}
{
    func = c;
}

JsonWrapper::Function::Function(const char *n, const char *l, void (*c)(const char *p)) : name{n}, label{l}
{
    func = c;
}

bool JsonWrapper::Function::isSameName(const char *n)
{
    return strcmp(name, n) == 0;
}

bool JsonWrapper::Function::isSameLabel(const char *l)
{
    return strcmp(label, l) == 0;
}

void JsonWrapper::Function::callback(JsonObject &_doc)
{
    Serial.println("gpio callback");
    const char *n = _doc["name"];
    Serial.println(name);
    Serial.println(n);
    // bool isMatchType = _doc["val"].is<T>;
    if (strcmp(name, n) == 0 && _doc["val"].is<bool>())
    {
        const char *v = _doc["val"];
        Serial.println(v);
        Serial.println("Correct json object, call callback");
        // Serial.println(*func);
        if (func)
        {
            Serial.println("tehe");
            func(v);
        }
        else
        {
            Serial.println(":(");
        }
    }
    else
    {
        Serial.println("Failed json object");
    }
}

JsonWrapper::JsonWrapper()
{
    Serial.println("JsonWrapper");
}

void JsonWrapper::gpio(const char *name, int pin, const char *label, bool enable)
{
    gpios[gpio_index] = new Gpio(pin, name, label, enable);
    gpio_index++;
}
void JsonWrapper::gpio(const char *name, int pin, const char *label, void (*callback)(bool))
{
    gpios[gpio_index] = new Gpio(pin, name, label, *callback);
    gpio_index++;
}



void JsonWrapper::function(const char *name, void (*callback)(const char *))
{
    functions[variables_index] = new Function(name, *callback);
    functions_index++;
}

void JsonWrapper::listen(ListenerCallback callback)
{
    DynamicJsonDocument doc(OUTPUT_BUFFER_SIZE);
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < gpio_index; i++)
    {
        if (gpios[i]->isUpdate())
        {
            Serial.println("GPIO changed!");
            gpios[i]->addToJson(this, array);
        }
    }

    for (int i = 0; i < variables_index; i++)
    {

        if (variables[i]->isUpdate())
        {
            Serial.println("Variable changed!");
            variables[i]->addToJson(this, array);
        }
    }
    if (array.size() > 0)
    {
        Serial.println(F("New Data JSON"));
        // serializeJsonPretty(array, Serial);
        int len = measureJson(array);

        char buffer[len + 100];

        serializeJson(array, buffer, len + 100);
        callback(buffer);
    }
}

void JsonWrapper::consume(const char *payload)
{
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    bool isArray = doc.is<JsonArray>();
    Serial.println(isArray);

    int len = doc.size();

    if (isArray)
    {

        for (int i = 0; i < len; i++)
        {
            JsonObject obj = doc[i];
            serializeJsonPretty(obj, Serial);
            process(obj);
        }
    }
    else
    {
        JsonObject obj = doc.as<JsonObject>();
        serializeJsonPretty(obj, Serial);
        process(obj);
    }
}

void JsonWrapper::process(JsonObject &doc)
{
    const char *type = doc["type"];

    if (strcmp(type, "variable") == 0)
    {
        const char *name = doc["name"];
        Serial.println(name);
        for (int i = 0; i < variables_index; i++)
        {

            if (variables[i]->isSameName(name))
            {
                // Serial.println("same name!");
                variables[i]->callback(doc);
            }
            else
            {
                // Serial.println("not same name :(");
            }
        }
    }
    else if (strcmp(type, "gpio") == 0)
    {
        const char *name = doc["name"];
        Serial.println(name);
        for (int i = 0; i < gpio_index; i++)
        {

            if (gpios[i]->isSameName(name))
            {
                Serial.println("same name!");
                gpios[i]->callback(doc);
            }
            else
            {
                Serial.println("not same name :(");
            }
        }
    }
}

const char *JsonWrapper::getByName(const char *name)
{
    DynamicJsonDocument doc(OUTPUT_BUFFER_SIZE);
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < gpio_index; i++)
    {
        if (gpios[i]->isSameName(name))
        {
            Serial.println("GPIO has same name");
            gpios[i]->addToJson(this, array);
        }
    }

    for (int i = 0; i < variables_index; i++)
    {

        if (variables[i]->isSameName(name))
        {
            Serial.println("Variable has same name");
            variables[i]->addToJson(this, array);
        }
    }
    if (array.size() > 0)
    {
        Serial.println(F("New Data JSON"));
        // serializeJsonPretty(array, Serial);
        int len = measureJson(array);

        char *buffer = new char[len + 100];

        serializeJson(array, buffer, len + 100);
        // callback(buffer);
        return buffer;
    }

    return "";
}

const char *JsonWrapper::getByLabel(const char *label)
{
    DynamicJsonDocument doc(OUTPUT_BUFFER_SIZE);
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < gpio_index; i++)
    {
        if (gpios[i]->isSameLabel(label))
        {
            Serial.println(F("GPIO has same label"));
            gpios[i]->addToJson(this, array);
        }
    }

    for (int i = 0; i < variables_index; i++)
    {

        if (variables[i]->isSameLabel(label))
        {
            Serial.println(F("Variable has same label"));
            variables[i]->addToJson(this, array);
        }
    }
    if (array.size() > 0)
    {
        Serial.println(F("New Data JSON"));
        // serializeJsonPretty(array, Serial);
        int len = measureJson(array);

        char *buffer = new char[len + 100];

        serializeJson(array, buffer, len + 100);
        // callback(buffer);
        return buffer;
    }

    return "";
}



void JsonWrapper::addGpioToJson(bool toAdd, const char *name, int pin, bool enable, const char *label, const JsonArray &_array)
{
    StaticJsonDocument<600> doc;

    JsonObject object = doc.to<JsonObject>();
    object["type"] = "gpio";
    object["name"] = name;
    object["pin"] = pin;
    object["val"] = toAdd;
    object["en"] = enable;
    object["lbl"] = label;
    object["ts"] = millis();

    _array.add(object);
}