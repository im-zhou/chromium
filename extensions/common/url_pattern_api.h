#ifndef EXTENSIONS_COMMON_UPM_EXPORT_H_
#define EXTENSIONS_COMMON_UPM_EXPORT_H_

#if defined(WIN32)
#define UPM_EXPORT __declspec(dllexport)
#else
#define UPM_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef const char* String;

typedef class URLPattern* URLPatternPtr;

namespace extensions {

typedef class URLPatternSet* URLPatternSetPtr;

}  // namespace extensions

namespace upa {

typedef enum SchemeMasks {
  SCHEME_NONE = 0,
  SCHEME_HTTP = 1 << 0,
  SCHEME_HTTPS = 1 << 1,
  SCHEME_FILE = 1 << 2,
  SCHEME_FTP = 1 << 3,
  SCHEME_WS = 1 << 7,
  SCHEME_WSS = 1 << 8,
  SCHEME_DATA = 1 << 9,
} SchemeMasks;

typedef enum ParseResult {
  kSuccess = 0,
  kMissingSchemeSeparator,
  kInvalidScheme,
  kWrongSchemeSeparator,
  kEmptyHost,
  kInvalidHostWildcard,
  kEmptyPath,
  kInvalidPort,
  kInvalidHost,
  kNumParseResults,
} ParseResult;

}  // namespace upa

UPM_EXPORT URLPatternPtr URLPatternCreate(int valid_schemes);
UPM_EXPORT void URLPatternDestroy(URLPatternPtr self);
UPM_EXPORT upa::ParseResult URLPatternParse(URLPatternPtr self, String pattern);
UPM_EXPORT bool URLPatternMatchesURL(URLPatternPtr self, String url);
UPM_EXPORT bool URLPatternOverlaps(URLPatternPtr self, URLPatternPtr other);
UPM_EXPORT bool URLPatternContains(URLPatternPtr self, URLPatternPtr other);
UPM_EXPORT String URLPatternToString(URLPatternPtr self);
UPM_EXPORT String ParseResultToString(upa::ParseResult parse_result);

UPM_EXPORT extensions::URLPatternSetPtr URLPatternSetCreate();
UPM_EXPORT void URLPatternSetDestroy(extensions::URLPatternSetPtr self);
UPM_EXPORT bool URLPatternSetIsEmpty(extensions::URLPatternSetPtr self);
UPM_EXPORT unsigned long URLPatternSetSize(extensions::URLPatternSetPtr self);
UPM_EXPORT bool URLPatternSetAdd(extensions::URLPatternSetPtr self,
                                 URLPatternPtr pattern);
UPM_EXPORT void URLPatternSetAddSet(extensions::URLPatternSetPtr self,
                                    extensions::URLPatternSetPtr set);
UPM_EXPORT void URLPatternSetClear(extensions::URLPatternSetPtr self);
UPM_EXPORT bool URLPatternSetHas(extensions::URLPatternSetPtr self,
                                 URLPatternPtr pattern);
UPM_EXPORT bool URLPatternSetMatchURL(extensions::URLPatternSetPtr self,
                                      String url);
UPM_EXPORT bool URLPatternSetOverlaps(extensions::URLPatternSetPtr self,
                                      extensions::URLPatternSetPtr other);
UPM_EXPORT bool URLPatternSetContains(extensions::URLPatternSetPtr self,
                                      extensions::URLPatternSetPtr other);

#ifdef __cplusplus
}
#endif

#endif  // EXTENSIONS_COMMON_UPM_EXPORT_H_
