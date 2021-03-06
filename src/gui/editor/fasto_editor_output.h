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

#include "fasto_editor.h"

#define JSON 0
#define CSV 1
#define RAW 2
#define HEX 3
#define MSGPACK 4
#define GZIP 5

namespace fastonosql {
namespace gui {
class FastoHexEdit;
}
}  // lines 50-50

namespace fastonosql {
namespace gui {
class FastoEditorOutput : public QWidget {
  Q_OBJECT
 public:
  explicit FastoEditorOutput(const QString& delimiter, QWidget* parent = 0);

  void setModel(QAbstractItemModel* model);

  QModelIndex selectedItem(int column) const;
  bool setData(const QModelIndex& index, const QVariant& value, int role);
  int viewMethod() const;
  QString text() const;
  bool isReadOnly() const;
  int childCount() const;

 Q_SIGNALS:
  void textChanged();
  void readOnlyChanged();

 public Q_SLOTS:
  void setReadOnly(bool ro);
  void viewChange(int viewMethod);

 private Q_SLOTS:
  void modelDestroyed();
  void dataChanged(QModelIndex first, QModelIndex last);
  void headerDataChanged();
  void rowsInserted(QModelIndex index, int r, int c);
  void rowsAboutToBeRemoved(QModelIndex index, int r, int c);
  void rowsRemoved(QModelIndex index, int r, int c);
  void columnsAboutToBeRemoved(QModelIndex index, int r, int c);
  void columnsRemoved(QModelIndex index, int r, int c);
  void columnsInserted(QModelIndex index, int r, int c);
  void reset();
  void layoutChanged();

 private:
  FastoHexEdit* editor_;
  QAbstractItemModel* model_;
  int view_method_;
  const QString delimiter_;
};
}  // namespace gui
}  // namespace fastonosql
