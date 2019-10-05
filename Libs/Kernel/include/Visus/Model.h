/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#ifndef VISUS_MODEL_H__
#define VISUS_MODEL_H__

#include <Visus/Kernel.h>
#include <Visus/Time.h>
#include <Visus/StringUtils.h>
#include <Visus/Log.h>
#include <Visus/SignalSlot.h>
#include <Visus/ApplicationInfo.h>

#include <stack>
#include <fstream>

namespace Visus {

//////////////////////////////////////////////////////////
class VISUS_KERNEL_API BaseView
{
public:

  VISUS_CLASS(BaseView)

  virtual ~BaseView() {
  }
};


//////////////////////////////////////////////////////////
class VISUS_KERNEL_API Model
{
public:

  VISUS_NON_COPYABLE_CLASS(Model)

  Signal<void()> begin_update;

  //end_update
  Signal<void()> end_update;

  //destroyed
  Signal<void()> destroyed;

  //views
  std::vector<BaseView*> views;

  //constructor
  Model();

  //destructor
  virtual ~Model();

  //getTypeName
  virtual String getTypeName() const = 0;

public:

  //enableLog
  void enableLog(String filename);

  //clearHistory
  void clearHistory();

  //getHistory
  const StringTree& getHistory() const {
    return history;
  }

public:

  //isUpdating
  inline bool isUpdating() const {
    return redo_stack.size() > 0;
  }

  //topRedo
  StringTree& topRedo() {
    return redo_stack.top();
  }

  //topUndo
  StringTree& topUndo() {
    return undo_stack.top();
  }

  //beginUpdate
  void beginUpdate(StringTree redo, StringTree undo);

  //endUpdate
  void endUpdate();

  //beginTransaction
  void beginTransaction() {
    beginUpdate(Transaction(),Transaction());
  }

  //endTransaction
  void endTransaction() {
    endUpdate();
  }

  //Transaction
  static StringTree Transaction() {
    return StringTree("transaction");
  }

  //isTransaction
  bool isTransaction(const StringTree& action) const {
    return action.name == "transaction";
  }

  //Diff
  static StringTree Diff() {
    return StringTree("diff");
  }

  //isDiff
  bool isDiff(const StringTree& action) const {
    return action.name == "diff";;
  }

public:

  //setProperty
  template <typename Value>
  void setProperty(String target_id, Value& old_value, const Value& new_value)
  {
    if (old_value == new_value) return;
    beginUpdate(
      createPassThroughAction(StringTree("set"), target_id).write("value", new_value),
      createPassThroughAction(StringTree("set"), target_id).write("value", old_value));
    {
      old_value = new_value;
    }
    endUpdate();
  }

  //setEncodedProperty
  template <typename Value>
  void setEncodedProperty(String target_id, Value& old_value, const Value& new_value)
  {
    if (old_value == new_value) return;
    beginUpdate(
      createPassThroughAction(EncodeObject(new_value, "set"), target_id),
      createPassThroughAction(EncodeObject(old_value, "set"), target_id));
    {
      old_value = new_value;
    }
    endUpdate();
  }


  //popTargetId
  String popTargetId(StringTree& action);

  //pushTargetId
  void pushTargetId(StringTree& action, String target_id);

  //createPassThroughAction
  StringTree createPassThroughAction(StringTree action, String target_id);

  //getPassThroughAction
  bool getPassThroughAction(StringTree& action, String match);

public:

  //copy
  static void copy(Model& dst, StringTree redo);

  //copy
  static void copy(Model& dst, const Model& src);

    //executeAction
  virtual void executeAction(StringTree action);

public:

  //canRedo
  bool canRedo() const {
    return !undo_redo.empty() && cursor_undo_redo < undo_redo.size();
  }

  //canUndo
  bool canUndo() const {
    return !undo_redo.empty() && cursor_undo_redo > 0;
  }

  //redo
  bool redo();

  //undo
  bool undo();

public:

  //addView
  void addView(BaseView* value) {
    this->views.push_back(value);
  }

  //removeView
  void removeView(BaseView* value) {
    Utils::remove(this->views,value);
  }

public:

  //writeTo
  virtual void writeTo(StringTree& out) const  = 0;

  //readFrom
  virtual void readFrom(StringTree& in) = 0;

protected:

  //modelChanged
  virtual void modelChanged() {
  }

private:

  typedef std::pair<StringTree, StringTree> UndoRedo;

  StringTree               history;
  String                   log_filename;
  std::ofstream            log;
  std::stack<StringTree>   redo_stack;
  std::stack<StringTree>   undo_stack;
  std::vector<UndoRedo>    undo_redo;
  int                      cursor_undo_redo = 0;
  bool                     bUndoing = false;
  bool                     bRedoing = false;
  Int64                    utc=0;
  StringTree               diff_begin;

};


//encode
inline VISUS_KERNEL_API StringTree EncodeObject(Model& model, String root_name = "") {
  return EncodeObject<Model>(model, root_name.empty() ? model.getTypeName() : root_name);
}

//////////////////////////////////////////////////////////
template <class ModelClassArg>
class View : public virtual BaseView
{
public:

  typedef ModelClassArg ModelClass;

  //constructor
  View() : model(nullptr)
  {}

  //destructor
  virtual ~View()
  {
    //did you forget to call bindModel(nullptr)?
    VisusAssert(model==nullptr);
    this->View::bindModel(nullptr);
  }

  //getModel
  inline ModelClass* getModel() const {
    return model;
  }

  //bindViewModel
  virtual void bindModel(ModelClass* value)
  {
    if (value==this->model) return;

    if (this->model) 
    {
      this->model->removeView(this);
      this->model->end_update.disconnect(changed_slot);
      this->model->Model::destroyed.disconnect(destroyed_slot);
    }

    this->model=value; 

    if (this->model) 
    {
      this->model->end_update.connect(changed_slot=Slot<void()>([this] {
        modelChanged();
      }));

      this->model->Model::destroyed.connect(destroyed_slot=Slot<void()>([this] {
        bindModel(nullptr);
      }));

      this->model->addView(this);
    }
  }

  //rebindModel
  inline void rebindModel()
  {
    ModelClass* model=this->model;
    bindModel(nullptr);
    bindModel(model);
  }

  //modelChanged
  virtual void modelChanged(){
  }

protected:

  ModelClass* model;

private:

  VISUS_NON_COPYABLE_CLASS(View)

  Slot<void()> changed_slot;
  Slot<void()> destroyed_slot;

};

} //namespace Visus

#endif //VISUS_MODEL_H__


