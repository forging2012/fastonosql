/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    FastoNoSQL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoNoSQL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoNoSQL.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "core/db/lmdb/internal/commands_api.h"

namespace fastonosql {
namespace core {
namespace lmdb {

common::Error CommandsApi::Info(internal::CommandHandler* handler,
                                int argc,
                                const char** argv,
                                FastoObject* out) {
  DBConnection* mdb = static_cast<DBConnection*>(handler);
  ServerInfo::Stats statsout;
  common::Error err = mdb->Info(argc == 1 ? argv[0] : nullptr, &statsout);
  if (err && err->IsError()) {
    return err;
  }

  common::StringValue* val = common::Value::CreateStringValue(ServerInfo(statsout).ToString());
  FastoObject* child = new FastoObject(out, val, mdb->Delimiter());
  out->AddChildren(child);
  return common::Error();
}

}  // namespace lmdb
}  // namespace core
}  // namespace fastonosql
