/*    Copyright 2015 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#pragma once

#include <cstring>
#include <tuple>
#include <type_traits>

#include "mongo/base/data_type.h"
#include "mongo/base/error_codes.h"
#include "mongo/base/status_with.h"
#include "mongo/platform/endian.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

    class ConstDataRange {

    public:
        // begin and end should point to the first and one past last bytes in
        // the range you wish to view.
        //
        // debug_offset provides a way to indicate that the ConstDataRange is
        // located at an offset into some larger logical buffer. By setting it
        // to a non-zero value, you'll change the Status messages that are
        // returned on failure to be offset by the amount passed to this
        // constructor.
        ConstDataRange(const char* begin, const char* end, std::ptrdiff_t debug_offset = 0)
            : _begin(begin), _end(end), _debug_offset(debug_offset) {
            invariant(end >= begin);
        }

        const char* data() const {
            return _begin;
        }

        size_t length() const {
            return _end - _begin;
        }

        template<typename T>
        Status read(T* t, size_t offset = 0) const {
            if (offset > length()) {
                return makeOffsetStatus(offset);
            }

            return DataType::load(t, _begin + offset, length() - offset, nullptr,
                                  offset + _debug_offset);
        }

        template<typename T>
        StatusWith<T> read(std::size_t offset = 0) const {
            T t(DataType::defaultConstruct<T>());
            Status s = read(&t, offset);

            if (s.isOK()) {
                return StatusWith<T>(std::move(t));
            } else {
                return StatusWith<T>(std::move(s));
            }
        }

        friend bool operator==(const ConstDataRange& lhs, const ConstDataRange& rhs) {
            return std::tie(lhs._begin, lhs._end) == std::tie(rhs._begin, rhs._end);
        }

        friend bool operator!=(const ConstDataRange& lhs, const ConstDataRange& rhs) {
            return !(lhs == rhs);
        }


    protected:
        const char* _begin;
        const char* _end;
        std::ptrdiff_t _debug_offset;

        Status makeOffsetStatus(size_t offset) const;

    };

    class DataRange : public ConstDataRange {

    public:
        typedef char* bytes_type;

        DataRange(bytes_type begin, bytes_type end, std::ptrdiff_t debug_offset = 0)
            : ConstDataRange(begin, end, debug_offset) {
        }

        template<typename T>
        Status write(const T& value, std::size_t offset = 0) {
            if (offset > length()) {
                return makeOffsetStatus(offset);
            }

            return DataType::store(value, const_cast<char *>(_begin + offset), length() - offset,
                                   nullptr, offset + _debug_offset);
        }

    };

} // namespace mongo
