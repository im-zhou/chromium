#include <iostream>

#include "extensions/common/url_pattern_api.h"

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: <url> <pattern>" << std::endl;
    return 2;
  }

  char* url_arg = argv[1];
  char* pattern_arg = argv[2];

  URLPatternPtr pattern = URLPatternCreate(upa::SchemeMasks::SCHEME_HTTP |
                                           upa::SchemeMasks::SCHEME_HTTPS);
  upa::ParseResult parseResult = URLPatternParse(pattern, pattern_arg);
  if (parseResult != upa::ParseResult::kSuccess) {
    std::cerr << "Can not parse pattern " << pattern_arg << ":\n\t"
              << ParseResultToString(parseResult) << std::endl;
    URLPatternDestroy(pattern);
    return 1;
  }

  extensions::URLPatternSetPtr patternSet = URLPatternSetCreate();
  bool res = URLPatternSetAdd(patternSet, pattern) &&
             URLPatternSetHas(patternSet, pattern) &&
             !URLPatternSetIsEmpty(patternSet) &&
             URLPatternSetSize(patternSet) > 0 &&
             URLPatternSetMatchURL(patternSet, url_arg);
  if (res) {
    std::cout << url_arg << " matches " << URLPatternToString(pattern)
              << std::endl;
  } else {
    std::cout << "No match" << std::endl;
  }

  URLPatternSetClear(patternSet);
  URLPatternSetDestroy(patternSet);
  URLPatternDestroy(pattern);
  return 1 - res;
}
