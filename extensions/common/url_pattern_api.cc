#include "extensions/common/url_pattern_api.h"

#include "base/check.h"
#include "extensions/common/url_pattern.h"
#include "extensions/common/url_pattern_set.h"
#include "url/gurl.h"

// C functions that forward calls to C++ implementation.

URLPatternPtr URLPatternCreate(int valid_schemes) {
  return new URLPattern(valid_schemes);
}

void URLPatternDestroy(URLPatternPtr self) {
  DCHECK(self);
  return delete self;
}

upa::ParseResult URLPatternParse(URLPatternPtr self, String pattern) {
  DCHECK(self);
  URLPattern::ParseResult res = self->Parse(pattern);
  return static_cast<upa::ParseResult>(res);
}

bool URLPatternMatchesURL(URLPatternPtr self, String url) {
  DCHECK(self);
  return self->MatchesURL(GURL(url));
}

bool URLPatternOverlaps(URLPatternPtr self, URLPatternPtr other) {
  DCHECK(self);
  return self->OverlapsWith(*other);
}

bool URLPatternContains(URLPatternPtr self, URLPatternPtr other) {
  DCHECK(self);
  return self->Contains(*other);
}

String URLPatternToString(URLPatternPtr self) {
  DCHECK(self);
  return self->GetAsString().c_str();
}

String ParseResultToString(upa::ParseResult parse_result) {
  return URLPattern::GetParseResultString(
      static_cast<URLPattern::ParseResult>(parse_result));
}

extensions::URLPatternSetPtr URLPatternSetCreate() {
  return new extensions::URLPatternSet();
}

void URLPatternSetDestroy(extensions::URLPatternSetPtr self) {
  DCHECK(self);
  return delete self;
}

bool URLPatternSetIsEmpty(extensions::URLPatternSetPtr self) {
  DCHECK(self);
  return self->is_empty();
}

unsigned long URLPatternSetSize(extensions::URLPatternSetPtr self) {
  DCHECK(self);
  return self->size();
}

bool URLPatternSetAdd(extensions::URLPatternSetPtr self,
                      URLPatternPtr pattern) {
  DCHECK(self);
  return self->AddPattern(*pattern);
}

void URLPatternSetAddSet(extensions::URLPatternSetPtr self,
                         extensions::URLPatternSetPtr set) {
  DCHECK(self);
  return self->AddPatterns(*set);
}

void URLPatternSetClear(extensions::URLPatternSetPtr self) {
  DCHECK(self);
  return self->ClearPatterns();
}

bool URLPatternSetHas(extensions::URLPatternSetPtr self,
                      URLPatternPtr pattern) {
  DCHECK(self);
  return self->ContainsPattern(*pattern);
}

bool URLPatternSetMatchURL(extensions::URLPatternSetPtr self,
                                      String url) {
  DCHECK(self);
  return self->MatchesURL(GURL(url));
}

bool URLPatternSetOverlaps(extensions::URLPatternSetPtr self,
                                      extensions::URLPatternSetPtr other) {
  DCHECK(self);
  return self->OverlapsWith(*other);
}

bool URLPatternSetContains(extensions::URLPatternSetPtr self,
                                      extensions::URLPatternSetPtr other) {
  DCHECK(self);
  return self->Contains(*other);
}
