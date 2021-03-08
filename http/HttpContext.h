//
// Created by gzp on 2021/3/6.
//

#ifndef DUODUO_HTTPCONTEXT_H
#define DUODUO_HTTPCONTEXT_H

#include "../buffer/Buffer.h"
#include "HttpRequest.h"
using namespace buffer;

namespace duoduo {
    namespace http {

        class HttpContext {
        public:
            enum HttpRequestParseState {
                kExpectRequestLine,
                kExpectHeaders,
                kExpectBody,
                kGotAll,
            };

            HttpContext()
                    : state_(kExpectRequestLine) {
            }


            // return false if any error
            bool parseRequest(Buffer *buf);

            bool gotAll() const { return state_ == kGotAll; }

            void reset() {
                state_ = kExpectRequestLine;
                HttpRequest dummy;
                request_.swap(dummy);
            }

            const HttpRequest &request() const { return request_; }

            HttpRequest &request() { return request_; }

        private:
            bool processRequestLine(const char *begin, const char *end);

            HttpRequestParseState state_;
            HttpRequest request_;
        };
    }
}



#endif //DUODUO_HTTPCONTEXT_H
