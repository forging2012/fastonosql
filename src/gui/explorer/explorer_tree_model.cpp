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

#include "gui/explorer/explorer_tree_model.h"

#include <memory>  // for __shared_ptr, operator==, etc
#include <string>  // for operator==, string, etc
#include <vector>  // for vector

#include <QIcon>

#include <common/convert2string.h>  // for ConvertFromString
#include <common/macros.h>          // for CHECK, NOTREACHED, etc
#include <common/net/types.h>       // for ConvertToString

#include <common/qt/utils_qt.h>  // for item
#include <common/qt/convert2string.h>
#include <common/qt/gui/base/tree_item.h>   // for TreeItem, findItemRecursive, etc
#include <common/qt/gui/base/tree_model.h>  // for TreeModel
#include <common/qt/logger.h>

#include "core/connection_types.h"        // for ConvertToString
#include "proxy/events/events_info.h"     // for CommandRequest, etc
#include "proxy/database/idatabase.h"     // for IDatabase
#include "proxy/server/iserver_local.h"   // for IServer, IServerRemote, etc
#include "proxy/server/iserver_remote.h"  // for IServer, IServerRemote, etc
#include "proxy/cluster/icluster.h"       // for ICluster
#include "proxy/sentinel/isentinel.h"     // for ISentinel, Sentinel, etc

#include "gui/gui_factory.h"  // for GuiFactory
#include "gui/explorer/explorer_tree_item.h"

#include "translations/global.h"  // for trName

namespace {
const QString trDiscoveryToolTipTemplate_3S = QObject::tr(
    "<b>Name:</b> %1<br/>"
    "<b>Type:</b> %2<br/>"
    "<b>Host:</b> %3<br/>");
const QString trRemoteServerToolTipTemplate_4S = QObject::tr(
    "<b>Name:</b> %1<br/>"
    "<b>Type:</b> %2<br/>"
    "<b>Mode:</b> %3<br/>"
    "<b>Host:</b> %4<br/>");
const QString trLocalServerToolTipTemplate_2S = QObject::tr(
    "<b>Name:</b> %1<br/>"
    "<b>Path:</b> %3<br/>");
const QString trDbToolTipTemplate_1S = QObject::tr("<b>Db size:</b> %1 keys<br/>");
const QString trNamespace_1S = QObject::tr("<b>Group size:</b> %1 keys<br/>");
}  // namespace

namespace fastonosql {
namespace gui {
ExplorerTreeModel::ExplorerTreeModel(QObject* parent) : TreeModel(parent) {}

QVariant ExplorerTreeModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  IExplorerTreeItem* node = common::qt::item<common::qt::gui::TreeItem*, IExplorerTreeItem*>(index);
  if (!node) {
    NOTREACHED();
    return QVariant();
  }

  int col = index.column();
  IExplorerTreeItem::eType type = node->type();

  if (role == Qt::ToolTipRole) {
    if (type == IExplorerTreeItem::eServer) {
      ExplorerServerItem* server_node = static_cast<ExplorerServerItem*>(node);
      proxy::IServerSPtr server = server_node->server();
      QString sname;
      common::ConvertFromString(server->Name(), &sname);
      bool is_can_remote = server->IsCanRemote();
      if (is_can_remote) {
        proxy::IServerRemote* rserver = dynamic_cast<proxy::IServerRemote*>(server.get());  // +
        CHECK(rserver);
        QString stype;
        common::ConvertFromString(common::ConvertToString(rserver->Role()), &stype);
        QString mtype;
        common::ConvertFromString(common::ConvertToString(rserver->Mode()), &mtype);
        QString shost;
        common::ConvertFromString(common::ConvertToString(rserver->Host()), &shost);
        return trRemoteServerToolTipTemplate_4S.arg(sname, stype, mtype, shost);
      } else {
        proxy::IServerLocal* lserver = dynamic_cast<proxy::IServerLocal*>(server.get());  // +
        CHECK(lserver);
        QString spath;
        common::ConvertFromString(lserver->Path(), &spath);
        return trLocalServerToolTipTemplate_2S.arg(sname, spath);
      }
    } else if (type == IExplorerTreeItem::eDatabase) {
      ExplorerDatabaseItem* db = static_cast<ExplorerDatabaseItem*>(node);
      if (db->isDefault()) {
        return trDbToolTipTemplate_1S.arg(db->totalKeysCount());
      }
    } else if (type == IExplorerTreeItem::eNamespace) {
      ExplorerNSItem* ns = static_cast<ExplorerNSItem*>(node);
      return trNamespace_1S.arg(ns->keyCount());
    }

    return QVariant();
  }

  if (role == Qt::DecorationRole && col == ExplorerServerItem::eName) {
    if (type == IExplorerTreeItem::eCluster) {
      return GuiFactory::Instance().clusterIcon();
    } else if (type == IExplorerTreeItem::eSentinel) {
      return GuiFactory::Instance().sentinelIcon();
    } else if (type == IExplorerTreeItem::eServer) {
      ExplorerServerItem* server_node = static_cast<ExplorerServerItem*>(node);
      proxy::IServerSPtr server = server_node->server();
      return GuiFactory::Instance().icon(server->Type());
    } else if (type == IExplorerTreeItem::eKey) {
      ExplorerKeyItem* key = static_cast<ExplorerKeyItem*>(node);
      core::NKey nkey = key->key();
      if (nkey.TTL() == NO_TTL) {
        return GuiFactory::Instance().keyIcon();
      } else {
        return GuiFactory::Instance().keyTTLIcon();
      }
    } else if (type == IExplorerTreeItem::eDatabase) {
      return GuiFactory::Instance().databaseIcon();
    } else if (type == IExplorerTreeItem::eNamespace) {
      return GuiFactory::Instance().directoryIcon();
    } else {
      NOTREACHED();
    }
  }

  if (role == Qt::DisplayRole) {
    if (col == IExplorerTreeItem::eName) {
      if (type == IExplorerTreeItem::eKey) {
        return node->name();
      } else if (type == IExplorerTreeItem::eDatabase) {
        ExplorerDatabaseItem* db = static_cast<ExplorerDatabaseItem*>(node);
        return QString("%1 (%2/%3)")
            .arg(node->name())
            .arg(db->loadedKeysCount())
            .arg(db->totalKeysCount());  // db
      } else if (type == IExplorerTreeItem::eNamespace) {
        ExplorerNSItem* ns = static_cast<ExplorerNSItem*>(node);
        return QString("%1 (%2)").arg(node->name()).arg(ns->keyCount());  // db
      } else {
        return QString("%1 (%2)").arg(node->name()).arg(node->childrenCount());  // server, cluster
      }
    }
  }

  if (role == Qt::ForegroundRole) {
    if (type == IExplorerTreeItem::eDatabase) {
      ExplorerDatabaseItem* db = static_cast<ExplorerDatabaseItem*>(node);
      if (db->isDefault()) {
        return QVariant(QColor(Qt::red));
      }
    }
  }

  return QVariant();
}

Qt::ItemFlags ExplorerTreeModel::flags(const QModelIndex& index) const {
  if (index.isValid()) {
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  }

  return Qt::NoItemFlags;
}

QVariant ExplorerTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    if (section == ExplorerServerItem::eName) {
      return translations::trName;
    }
  }

  return TreeModel::headerData(section, orientation, role);
}

int ExplorerTreeModel::columnCount(const QModelIndex& parent) const {
  UNUSED(parent);

  return ExplorerServerItem::eCountColumns;
}

void ExplorerTreeModel::addCluster(proxy::IClusterSPtr cluster) {
  if (!cluster) {
    return;
  }

  ExplorerClusterItem* cl = findClusterItem(cluster);
  if (!cl) {
    common::qt::gui::TreeItem* parent = root_;
    CHECK(parent);

    ExplorerClusterItem* item = new ExplorerClusterItem(cluster, parent);
    insertItem(QModelIndex(), item);
  }
}

void ExplorerTreeModel::removeCluster(proxy::IClusterSPtr cluster) {
  if (!cluster) {
    return;
  }

  ExplorerClusterItem* serverItem = findClusterItem(cluster);
  if (serverItem) {
    removeItem(QModelIndex(), serverItem);
  }
}

void ExplorerTreeModel::addServer(proxy::IServerSPtr server) {
  if (!server) {
    return;
  }

  ExplorerServerItem* serv = findServerItem(server.get());
  if (!serv) {
    common::qt::gui::TreeItem* parent = root_;
    CHECK(parent);

    ExplorerServerItem* item = new ExplorerServerItem(server, parent);
    insertItem(QModelIndex(), item);
  }
}

void ExplorerTreeModel::removeServer(proxy::IServerSPtr server) {
  if (!server) {
    return;
  }

  ExplorerServerItem* serverItem = findServerItem(server.get());
  if (serverItem) {
    removeItem(QModelIndex(), serverItem);
  }
}

void ExplorerTreeModel::addSentinel(proxy::ISentinelSPtr sentinel) {
  if (!sentinel) {
    return;
  }

  ExplorerSentinelItem* cl = findSentinelItem(sentinel);
  if (!cl) {
    common::qt::gui::TreeItem* parent = root_;
    CHECK(parent);

    ExplorerSentinelItem* item = new ExplorerSentinelItem(sentinel, parent);
    insertItem(QModelIndex(), item);
  }
}

void ExplorerTreeModel::removeSentinel(proxy::ISentinelSPtr sentinel) {
  if (!sentinel) {
    return;
  }

  ExplorerSentinelItem* serverItem = findSentinelItem(sentinel);
  if (serverItem) {
    removeItem(QModelIndex(), serverItem);
  }
}

void ExplorerTreeModel::addDatabase(proxy::IServer* server, core::IDataBaseInfoSPtr db) {
  ExplorerServerItem* parent = findServerItem(server);
  if (!parent) {
    return;
  }

  ExplorerDatabaseItem* dbs = findDatabaseItem(parent, db);
  if (!dbs) {
    common::qt::gui::TreeItem* parent_server = parent->parent();
    QModelIndex parent_index = createIndex(parent_server->indexOf(parent), 0, parent);
    ExplorerDatabaseItem* item = new ExplorerDatabaseItem(server->CreateDatabaseByInfo(db), parent);
    insertItem(parent_index, item);
  }
}

void ExplorerTreeModel::removeDatabase(proxy::IServer* server, core::IDataBaseInfoSPtr db) {
  ExplorerServerItem* parent = findServerItem(server);
  if (!parent) {
    return;
  }

  ExplorerDatabaseItem* dbs = findDatabaseItem(parent, db);
  if (dbs) {
    common::qt::gui::TreeItem* parent_server = parent->parent();
    QModelIndex index = createIndex(parent_server->indexOf(parent), 0, dbs);
    removeItem(index.parent(), dbs);
  }
}

void ExplorerTreeModel::setDefaultDb(proxy::IServer* server, core::IDataBaseInfoSPtr db) {
  ExplorerServerItem* parent = findServerItem(server);
  if (!parent) {
    return;
  }

  ExplorerDatabaseItem* dbs = findDatabaseItem(parent, db);
  if (!dbs) {
    DNOTREACHED();
    return;
  }

  QModelIndex parent_index =
      createIndex(root_->indexOf(parent), ExplorerDatabaseItem::eName, parent);
  QModelIndex dbs_last_index =
      index(parent->childrenCount(), ExplorerDatabaseItem::eCountColumns, parent_index);
  updateItem(parent_index, dbs_last_index);
}

void ExplorerTreeModel::updateDb(proxy::IServer* server, core::IDataBaseInfoSPtr db) {
  ExplorerServerItem* parent = findServerItem(server);
  if (!parent) {
    return;
  }

  ExplorerDatabaseItem* dbs = findDatabaseItem(parent, db);
  if (!dbs) {
    return;
  }

  int index_db = parent->indexOf(dbs);
  QModelIndex dbs_index1 = createIndex(index_db, ExplorerDatabaseItem::eName, dbs);
  QModelIndex dbs_index2 = createIndex(index_db, ExplorerDatabaseItem::eCountColumns, dbs);
  updateItem(dbs_index1, dbs_index2);
}

void ExplorerTreeModel::addKey(proxy::IServer* server,
                               core::IDataBaseInfoSPtr db,
                               const core::NDbKValue& dbv,
                               const std::string& ns_separator) {
  ExplorerServerItem* parent = findServerItem(server);
  if (!parent) {
    return;
  }

  ExplorerDatabaseItem* dbs = findDatabaseItem(parent, db);
  if (!dbs) {
    return;
  }

  core::NKey key = dbv.Key();
  ExplorerKeyItem* keyit = findKeyItem(dbs, key);
  if (!keyit) {
    IExplorerTreeItem* nitem = dbs;
    core::KeyInfo kinf = key.Info(ns_separator);
    if (kinf.HasNamespace()) {
      nitem = findOrCreateNSItem(dbs, kinf);
    }

    common::qt::gui::TreeItem* parent_nitem = nitem->parent();
    QModelIndex parent_index = createIndex(parent_nitem->indexOf(nitem), 0, nitem);
    ExplorerKeyItem* item = new ExplorerKeyItem(dbv, nitem);
    insertItem(parent_index, item);
  }
}

void ExplorerTreeModel::removeKey(proxy::IServer* server,
                                  core::IDataBaseInfoSPtr db,
                                  const core::NKey& key) {
  ExplorerServerItem* parent = findServerItem(server);
  if (!parent) {
    return;
  }

  ExplorerDatabaseItem* dbs = findDatabaseItem(parent, db);
  if (!dbs) {
    return;
  }

  ExplorerKeyItem* keyit = findKeyItem(dbs, key);
  if (keyit) {
    common::qt::gui::TreeItem* par = keyit->parent();
    QModelIndex index = createIndex(par->indexOf(keyit), 0, keyit);
    removeItem(index.parent(), keyit);
  }
}

void ExplorerTreeModel::updateKey(proxy::IServer* server,
                                  core::IDataBaseInfoSPtr db,
                                  const core::NKey& old_key,
                                  const core::NKey& new_key) {
  ExplorerServerItem* parent = findServerItem(server);
  if (!parent) {
    return;
  }

  ExplorerDatabaseItem* dbs = findDatabaseItem(parent, db);
  if (!dbs) {
    return;
  }

  ExplorerKeyItem* keyit = findKeyItem(dbs, old_key);
  if (keyit) {
    common::qt::gui::TreeItem* par = keyit->parent();
    int index_key = par->indexOf(keyit);
    keyit->setKey(new_key);
    QModelIndex key_index1 = createIndex(index_key, ExplorerKeyItem::eName, dbs);
    QModelIndex key_index2 = createIndex(index_key, ExplorerKeyItem::eCountColumns, dbs);
    updateItem(key_index1, key_index2);
  }
}

void ExplorerTreeModel::updateValue(proxy::IServer* server,
                                    core::IDataBaseInfoSPtr db,
                                    const core::NDbKValue& dbv) {
  ExplorerServerItem* parent = findServerItem(server);
  if (!parent) {
    return;
  }

  ExplorerDatabaseItem* dbs = findDatabaseItem(parent, db);
  if (!dbs) {
    return;
  }

  ExplorerKeyItem* keyit = findKeyItem(dbs, dbv.Key());
  if (keyit) {
    common::qt::gui::TreeItem* par = keyit->parent();
    int index_key = par->indexOf(keyit);
    keyit->setDbv(dbv);
    QModelIndex key_index1 = createIndex(index_key, ExplorerKeyItem::eName, dbs);
    QModelIndex key_index2 = createIndex(index_key, ExplorerKeyItem::eCountColumns, dbs);
    updateItem(key_index1, key_index2);
  }
}

void ExplorerTreeModel::removeAllKeys(proxy::IServer* server, core::IDataBaseInfoSPtr db) {
  ExplorerServerItem* parent = findServerItem(server);
  if (!parent) {
    return;
  }

  ExplorerDatabaseItem* dbs = findDatabaseItem(parent, db);
  if (!dbs) {
    return;
  };

  QModelIndex parentdb = createIndex(parent->indexOf(dbs), 0, dbs);
  removeAllItems(parentdb);
}

ExplorerClusterItem* ExplorerTreeModel::findClusterItem(proxy::IClusterSPtr cl) {
  common::qt::gui::TreeItem* parent = root_;
  if (!parent) {
    return nullptr;
  }

  for (size_t i = 0; i < parent->childrenCount(); ++i) {
    ExplorerClusterItem* item = dynamic_cast<ExplorerClusterItem*>(parent->child(i));  // +
    if (item && item->cluster() == cl) {
      return item;
    }
  }
  return nullptr;
}

ExplorerSentinelItem* ExplorerTreeModel::findSentinelItem(proxy::ISentinelSPtr sentinel) {
  common::qt::gui::TreeItem* parent = root_;
  if (!parent) {
    return nullptr;
  }

  for (size_t i = 0; i < parent->childrenCount(); ++i) {
    ExplorerSentinelItem* item = dynamic_cast<ExplorerSentinelItem*>(parent->child(i));  // +
    if (item && item->sentinel() == sentinel) {
      return item;
    }
  }
  return nullptr;
}

ExplorerServerItem* ExplorerTreeModel::findServerItem(proxy::IServer* server) const {
  return static_cast<ExplorerServerItem*>(
      common::qt::gui::findItemRecursive(root_, [server](common::qt::gui::TreeItem* item) -> bool {
        ExplorerServerItem* server_item = dynamic_cast<ExplorerServerItem*>(item);  // +
        if (!server_item) {
          return false;
        }

        return server_item->server().get() == server;
      }));
}

ExplorerDatabaseItem* ExplorerTreeModel::findDatabaseItem(ExplorerServerItem* server,
                                                          core::IDataBaseInfoSPtr db) const {
  if (!server) {
    DNOTREACHED();
    return nullptr;
  }

  for (size_t i = 0; i < server->childrenCount(); ++i) {
    ExplorerDatabaseItem* item = dynamic_cast<ExplorerDatabaseItem*>(server->child(i));  // +
    if (!item) {
      continue;
    }

    proxy::IDatabaseSPtr inf = item->db();
    if (inf && inf->Name() == db->Name()) {
      return item;
    }
  }

  return nullptr;
}

ExplorerKeyItem* ExplorerTreeModel::findKeyItem(IExplorerTreeItem* db_or_ns,
                                                const core::NKey& key) const {
  return static_cast<ExplorerKeyItem*>(
      common::qt::gui::findItemRecursive(db_or_ns, [key](common::qt::gui::TreeItem* item) -> bool {
        ExplorerKeyItem* key_item = dynamic_cast<ExplorerKeyItem*>(item);  // +
        if (!key_item) {
          return false;
        }

        core::NKey ckey = key_item->key();
        return ckey.Key() == key.Key();
      }));
}

ExplorerNSItem* ExplorerTreeModel::findNSItem(IExplorerTreeItem* db_or_ns,
                                              const QString& name) const {
  return static_cast<ExplorerNSItem*>(
      common::qt::gui::findItemRecursive(db_or_ns, [name](common::qt::gui::TreeItem* item) -> bool {
        ExplorerNSItem* ns_item = dynamic_cast<ExplorerNSItem*>(item);  // +
        if (!ns_item) {
          return false;
        }

        return ns_item->name() == name;
      }));
}

ExplorerNSItem* ExplorerTreeModel::findOrCreateNSItem(IExplorerTreeItem* db_or_ns,
                                                      const core::KeyInfo& kinf) {
  std::string nspace = kinf.Nspace();
  QString qnspace;
  common::ConvertFromString(nspace, &qnspace);
  ExplorerNSItem* founded_item = findNSItem(db_or_ns, qnspace);
  if (founded_item) {
    return founded_item;
  }

  size_t sz = kinf.NspaceSize();
  IExplorerTreeItem* par = db_or_ns;
  for (size_t i = 0; i < sz; ++i) {
    ExplorerNSItem* item = nullptr;
    nspace = kinf.JoinNamespace(i);
    common::ConvertFromString(nspace, &qnspace);
    for (size_t j = 0; j < par->childrenCount(); ++j) {
      ExplorerNSItem* ns_item = dynamic_cast<ExplorerNSItem*>(par->child(j));  // +
      if (!ns_item) {
        continue;
      }

      if (ns_item->name() == qnspace) {
        item = ns_item;
      }
    }

    if (!item) {
      common::qt::gui::TreeItem* gpar = par->parent();
      QModelIndex parentdb = createIndex(gpar->indexOf(par), 0, par);
      item = new ExplorerNSItem(qnspace, par);
      insertItem(parentdb, item);
    }

    par = item;
    founded_item = item;
  }

  return founded_item;
}
}  // namespace gui
}  // namespace fastonosql
