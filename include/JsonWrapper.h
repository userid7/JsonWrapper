#ifndef JsonWrapper_h
#define JsonWrapper_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

#define NUMBER_VARIABLES 10
#define OUTPUT_BUFFER_SIZE 2048

typedef std::function<void(const char *payload)> ListenerCallback;

class JsonWrapper
{
public:
    void gpio(const char *name, int pin, const char *label, bool enable);
    void gpio(const char *name, int pin, const char *label, void (*callback)(bool));

    template <typename T>
    void variable(const char *name, T *var, const char *label, void (*callback)(T));

    template <typename T>
    void variable(const char *name, T *var, const char *label, bool enable);

    void function(const char *name, void (*callback)(const char *));

    void listen(ListenerCallback callback);

    void consume(const char *payload);

    void process(JsonObject &doc);

    const char *getByName(const char *name);

    const char *getByLabel(const char *label);

    class Gpio
    {
    private:
        const char *name;
        int pin;
        bool lastState;
        void (*func)(bool) = 0;
        bool enable = false;

        const char *label;

    public:
        Gpio(int p, const char *n, const char *l, bool d);

        Gpio(int p, const char *n, const char *l, void (*c)(bool));

        void addToJson(JsonWrapper *web, JsonArray &_array);

        bool isUpdate();

        bool isSameName(const char *n);

        bool isSameLabel(const char *l);

        void callback(JsonObject &_doc);

    private:
        void defaultFunc(bool b);
    };

    class Variable
    {
    public:
        virtual void addToJson(JsonWrapper *web, JsonArray &_array) = 0;
        virtual bool isUpdate() const = 0;
        virtual bool isSameName(const char *n) const = 0;
        virtual bool isSameLabel(const char *n) const = 0;
        virtual void callback(JsonObject &_doc) const = 0;
    };

    template <typename T>
    class TypedVariable : public Variable
    {
    private:
        T *var;
        T lastVar;

        const char *name;
        void (*func)(T) = 0;
        bool enable = false;

        const char *label;

    public:
        TypedVariable(const char *n, T *v, const char *l, bool d);

        TypedVariable(const char *n, T *v, const char *l, void (*c)(T));

        void defaultFunc(T t) const;

        void addToJson(JsonWrapper *web, JsonArray &_array) override;

        bool isUpdate() const override;

        bool isSameName(const char *n) const override;

        bool isSameLabel(const char *l) const override;

        void callback(JsonObject &_doc) const override;
    };

    class Function
    {
    private:
        const char *name;
        void (*func)(const char *) = 0;
        const char *label;

    public:
        Function(const char *n, void (*c)(const char *p));

        Function(const char *n, const char *l, void (*c)(const char *p));

        bool isSameName(const char *n);

        bool isSameLabel(const char *l);

        void callback(JsonObject &_doc);
    };

    template <typename T>
    void addVariableToJson(T toAdd, const char *name, bool enable, const char *label, const JsonArray &_array);

    void addGpioToJson(bool toAdd, const char *name, int pin, bool enable, const char *label, const JsonArray &_array);

public:
    // Init gpio arrays
    uint8_t gpio_index;
    Gpio *gpios[NUMBER_VARIABLES];
    const char *gpio_names[NUMBER_VARIABLES];

    // Init variables arrays
    uint8_t variables_index;
    Variable *variables[NUMBER_VARIABLES];
    const char *variable_names[NUMBER_VARIABLES];

    // Init functions arrays
    uint8_t functions_index;
    Function *functions[NUMBER_VARIABLES];

public:
    JsonWrapper();
};

template <typename T>
JsonWrapper::TypedVariable<T>::TypedVariable(const char *n, T *v, const char *l, bool d) : var(v), name(n), label(l)
{
    lastVar = *var;
    enable = d;
}

template <typename T>
JsonWrapper::TypedVariable<T>::TypedVariable(const char *n, T *v, const char *l, void (*c)(T)) : var(v), name(n), label(l)
{
    lastVar = *var;
    func = c;
    enable = true;
}

template <typename T>
void JsonWrapper::TypedVariable<T>::defaultFunc(T t) const
{
    *var = t;
}

template <typename T>
void JsonWrapper::TypedVariable<T>::addToJson(JsonWrapper *web, JsonArray &_array)
{
    web->addVariableToJson(*var, name, enable, label, _array);
    lastVar = *var;
}

template <typename T>
bool JsonWrapper::TypedVariable<T>::isUpdate() const
{
    return lastVar != *var;
}

template <typename T>
bool JsonWrapper::TypedVariable<T>::isSameName(const char *n) const
{
    return strcmp(name, n) == 0;
}

template <typename T>
bool JsonWrapper::TypedVariable<T>::isSameLabel(const char *l) const
{
    return strcmp(label, l) == 0;
}

template <typename T>
void JsonWrapper::TypedVariable<T>::callback(JsonObject &_doc) const
{
    if (enable == true)
    {
        const char *n = _doc["name"];
        // Serial.println(name);
        // Serial.println(n);
        // bool isMatchType = _doc["val"].is<T>;
        if (strcmp(name, n) == 0 && _doc["val"].is<T>())
        {
            T v = _doc["val"];
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
        }
    }
}

template <typename T>
void JsonWrapper::variable(const char *name, T *var, const char *label, void (*callback)(T))
{
    variables[variables_index] = new TypedVariable<T>(name, var, label, *callback);
    variables_index++;
}

template <typename T>
void JsonWrapper::variable(const char *name, T *var, const char *label, bool enable)
{
    variables[variables_index] = new TypedVariable<T>(name, var, label, enable);
    variables_index++;
}

// Add to output buffer
template <typename T>
void JsonWrapper::addVariableToJson(T toAdd, const char *name, bool enable, const char *label, const JsonArray &_array)
{
    StaticJsonDocument<600> doc;

    JsonObject object = doc.to<JsonObject>();
    object["type"] = "variable";
    object["name"] = name;
    object["val"] = toAdd;
    object["en"] = enable;
    object["lbl"] = label;
    object["ts"] = millis();

    _array.add(object);
}

#endif