//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#pragma once

#include <nil/actor/http/handlers.hh>
#include <nil/actor/core/iostream.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            /**
             * This is a base class for file transformer.
             *
             * File transformer adds the ability to modify a file content before returning
             * the results, it acts as a factory class for output_stream.
             *
             * The transformer decides according to the file extension if transforming is
             * needed.
             *
             * If a transformation is needed it would create a new output stream from the given stream.
             */
            class file_transformer {
            public:
                /**
                 * Any file transformer should implement this method.
                 * @param req the request
                 * @param extension the file extension originating the content
                 * returns a new output stream to be used when writing the file to the reply
                 */
                virtual output_stream<char> transform(std::unique_ptr<request> req, const sstring &extension,
                                                      output_stream<char> &&s) = 0;

                virtual ~file_transformer() = default;
            };

            /**
             * A base class for handlers that interact with files.
             * directory and file handlers both share some common logic
             * with regards to file handling.
             * they both needs to read a file from the disk, optionally transform it,
             * and return the result or page not found on error
             */
            class file_interaction_handler : public handler_base {
            public:
                file_interaction_handler(file_transformer *p = nullptr) : transformer(p) {
                }

                ~file_interaction_handler();

                /**
                 * Allows setting a transformer to be used with the files returned.
                 * @param t the file transformer to use
                 * @return this
                 */
                file_interaction_handler *set_transformer(file_transformer *t) {
                    transformer = t;
                    return this;
                }

                /**
                 * if the url ends without a slash redirect
                 * @param req the request
                 * @param rep the reply
                 * @return true on redirect
                 */
                bool redirect_if_needed(const request &req, reply &rep) const;

                /**
                 * A helper method that returns the file extension.
                 * @param file the file to check
                 * @return the file extension
                 */
                static sstring get_extension(const sstring &file);

            protected:
                /**
                 * read a file from the disk and return it in the replay.
                 * @param file the full path to a file on the disk
                 * @param req the reuest
                 * @param rep the reply
                 */
                future<std::unique_ptr<reply>> read(sstring file, std::unique_ptr<request> req,
                                                    std::unique_ptr<reply> rep);
                file_transformer *transformer;

                output_stream<char> get_stream(std::unique_ptr<request> req, const sstring &extension,
                                               output_stream<char> &&s);
            };

            /**
             * The directory handler get a disk path in the
             * constructor.
             * and expect a path parameter in the handle method.
             * it would concatenate the two and return the file
             * e.g. if the path is /usr/mgmt/public in the path
             * parameter is index.html
             * handle will return the content of /usr/mgmt/public/index.html
             */
            class directory_handler : public file_interaction_handler {
            public:
                /**
                 * The directory handler map a base path and a path parameter to a file
                 * @param doc_root the root directory to search the file from.
                 * @param transformer an optional file transformer
                 * For example if the root is '/usr/mgmt/public' and the path parameter
                 * will be '/css/style.css' the file wil be /usr/mgmt/public/css/style.css'
                 */
                explicit directory_handler(const sstring &doc_root, file_transformer *transformer = nullptr);

                future<std::unique_ptr<reply>> handle(const sstring &path, std::unique_ptr<request> req,
                                                      std::unique_ptr<reply> rep) override;

            private:
                sstring doc_root;
            };

            /**
             * The file handler get a path to a file on the disk
             * in the constructor.
             * it will always return the content of the file.
             */
            class file_handler : public file_interaction_handler {
            public:
                /**
                 * The file handler map a file to a url
                 * @param file the full path to the file on the disk
                 * @param transformer an optional file transformer
                 * @param force_path check if redirect is needed upon `handle`
                 */
                explicit file_handler(const sstring &file, file_transformer *transformer = nullptr,
                                      bool force_path = true) :
                    file_interaction_handler(transformer),
                    file(file), force_path(force_path) {
                }

                future<std::unique_ptr<reply>> handle(const sstring &path, std::unique_ptr<request> req,
                                                      std::unique_ptr<reply> rep) override;

            private:
                sstring file;
                bool force_path;
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
