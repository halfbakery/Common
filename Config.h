/*
 * Config.h
 *
 *  Created on: Jul 12, 2016
 *      Author: lieven
 */

#ifndef ACTOR_CONFIG_H_
#define ACTOR_CONFIG_H_
#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_ENABLE_STD_STRING 1
#include <ArduinoJson.h>
#include <Log.h>

class Config {
		DynamicJsonDocument _jsonBuffer;
		JsonObject _root;
		std::string _nameSpace;
		bool _loaded;

	public:
		Config();
		virtual ~Config();

		void clear();
		void load();
		void save();
		void load(const char*);
		void save(std::string&);
		void loadFile(const char*);
		void saveFile(const char*);
		void printPretty(std::string& str);
		JsonObject root() { return _root;};

		bool hasKey(const char* key);
		Config&  setNameSpace(const char* ns);
		const char* getNameSpace();
		void remove(const char* key);

		template<typename T> void set(const char* key, T value) {
			_root[_nameSpace][key] = value;
		}
		template<typename T, typename T1> Config& get(const char* key, T& value,
		        T1 defaultValue) {
			if (_root[_nameSpace].containsKey(key))
				value = _root[_nameSpace][key];
			else {
				_root[_nameSpace][key] = defaultValue;
				value = defaultValue;
			}
			return *this;
		}
		Config& get(const char* key, std::string& value, const char* defaultValue) {
			if (_root[_nameSpace].containsKey(key))
				value = _root[_nameSpace][key].as<std::string>();
			else {
				_root[_nameSpace][key] = defaultValue;
				value = defaultValue;
			}
			return *this;
		}
};

extern Config config;

#endif /* ACTOR_CONFIG_H_ */
