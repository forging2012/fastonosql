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

#pragma once

#include <string>  // for string
#include <memory>  // for shared_ptr

#include <common/error.h>   // for Error
#include <common/macros.h>  // for WARN_UNUSED_RESULT
#include <common/value.h>   // for Value, Value::Type

#include "core/db_key.h"  // for NKey, NDbKValue, ttl_t
#include "core/db_ps_channel.h"
#include "core/command_holder.h"

#define FLUSHDB_COMMAND "FLUSHDB"
#define SELECTDB_COMMAND_1S "SELECT %s"

#define COMMONTYPE_GET_KEY_COMMAND "GET"
#define COMMONTYPE_SET_KEY_COMMAND "SET"

namespace fastonosql {
namespace core {

common::Error ParseCommands(const std::string& cmd, std::vector<std::string>* cmds);

class ICommandTranslator {
 public:
  explicit ICommandTranslator(const std::vector<CommandHolder>& commands);
  virtual ~ICommandTranslator();

  common::Error SelectDBCommand(const std::string& name,
                                std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error FlushDBCommand(std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error CreateKeyCommand(const NDbKValue& key,
                                 std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error LoadKeyCommand(const NKey& key,
                               common::Value::Type type,
                               std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error DeleteKeyCommand(const NKey& key, std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error RenameKeyCommand(const NKey& key,
                                 const std::string& new_name,
                                 std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error ChangeKeyTTLCommand(const NKey& key,
                                    ttl_t ttl,
                                    std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error LoadKeyTTLCommand(const NKey& key, std::string* cmdstring) const WARN_UNUSED_RESULT;

  bool IsLoadKeyCommand(const std::string& cmd, std::string* key) const WARN_UNUSED_RESULT;

  common::Error PublishCommand(const NDbPSChannel& channel,
                               const std::string& message,
                               std::string* cmdstring) const WARN_UNUSED_RESULT;

  common::Error SubscribeCommand(const NDbPSChannel& channel,
                                 std::string* cmdstring) const WARN_UNUSED_RESULT;

  std::vector<CommandInfo> Commands() const;
  common::Error FindCommand(int argc,
                            const char** argv,
                            const CommandHolder** info,
                            size_t* off) const WARN_UNUSED_RESULT;

  common::Error TestCommandArgs(const CommandHolder* cmd,
                                int argc_to_call,
                                const char** argv_to_call) const WARN_UNUSED_RESULT;
  common::Error TestCommandLine(const std::string& cmd) const WARN_UNUSED_RESULT;
  common::Error TestCommandLineArgs(int argc,
                                    const char** argv,
                                    const CommandHolder** info,
                                    size_t* off) const WARN_UNUSED_RESULT;

  static common::Error InvalidInputArguments(const std::string& cmd);
  static common::Error NotSupported(const std::string& cmd);
  static common::Error UnknownSequence(int argc, const char** argv);

 private:
  virtual common::Error CreateKeyCommandImpl(const NDbKValue& key,
                                             std::string* cmdstring) const = 0;
  virtual common::Error LoadKeyCommandImpl(const NKey& key,
                                           common::Value::Type type,
                                           std::string* cmdstring) const = 0;
  virtual common::Error DeleteKeyCommandImpl(const NKey& key, std::string* cmdstring) const = 0;
  virtual common::Error RenameKeyCommandImpl(const NKey& key,
                                             const std::string& new_name,
                                             std::string* cmdstring) const = 0;
  virtual common::Error ChangeKeyTTLCommandImpl(const NKey& key,
                                                ttl_t ttl,
                                                std::string* cmdstring) const = 0;
  virtual common::Error LoadKeyTTLCommandImpl(const NKey& key, std::string* cmdstring) const = 0;
  virtual common::Error PublishCommandImpl(const NDbPSChannel& channel,
                                           const std::string& message,
                                           std::string* cmdstring) const = 0;
  virtual common::Error SubscribeCommandImpl(const NDbPSChannel& channel,
                                             std::string* cmdstring) const = 0;

  virtual bool IsLoadKeyCommandImpl(const CommandInfo& cmd) const = 0;

  const std::vector<CommandHolder> commands_;
};

typedef common::shared_ptr<ICommandTranslator> translator_t;

}  // namespace core
}  // namespace fastonosql
