#include "ay_json.h"
#include <iostream>

class CB {
    public:
        int operator()(ay::JSON_SAX_parser& parser) {
            using namespace std;
            using namespace ay;
            switch(parser.d_event) {
                case JSON_SAX_parser::EVENT_OBJECT_START:
                    cout << "object start\n";
                    break;
                case JSON_SAX_parser::EVENT_OBJECT_END:
                    cout << "object end\n";
                    break;
                case JSON_SAX_parser::EVENT_ARRAY_START:
                    cout << "array start\n";
                    break;
                case JSON_SAX_parser::EVENT_ARRAY_END:
                    cout << "array end\n";
                    break;
                case JSON_SAX_parser::EVENT_NVPAIR_NAME:
                    cout << "nv pair\n";
                    break;
                case JSON_SAX_parser::EVENT_NVPAIR_VALUE_START:
                    cout << "nv pair value start\n";
                    break;
                case JSON_SAX_parser::EVENT_NVPAIR_VALUE_END:
                    cout << "nv pair value end\n";
                    break;
                case JSON_SAX_parser::EVENT_VALUE_ATOMIC:
                    cout << "nv atomic: ";
                    switch(parser.d_val.d_type) {
                        case JSON_value::VT_STRING:
                            cout << parser.d_val.getString() << endl;
                        default:
                            cout << endl;
                    }
                    break;
            }
            return 0;
        }
};

int main() {
    const char* json = "{\"a\": [{\"b\": 2}, 3.5, null]}";
    ay::JSON_SAX_parser p;
    CB c;
    p.parse(c, json, strlen(json));
}

