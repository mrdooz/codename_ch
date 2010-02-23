import os
import system_ext
import redux_ext
import dx_ext as dx
import traceback
import inspect
import sys
import pickle
import operator
import functools
#import jsonpickle
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyMimeData import PyMimeData

sys.path.append("effects")
sys.path.append("data/scenes")

print_materials = False
print_effects = False

texture_name = "diffuse_texture"

variable_manager = None
effect_manager = None
material_manager = None
system = None
editor = None

    
def refresh_variable_view(view):
    #view.reset()
    view.expandAll()
    view.resizeColumnToContents(0)
    view.resizeColumnToContents(1)

def inplace_filter(func, seq):
    """ A destructive filter function """
    i = len(seq) - 1
    while i >= 0:
        if not func(seq[i]):
            del seq[i]
        i -= 1

def classes_in_module(module_name):
    return [name for (name, type) in inspect.getmembers(module_name) if inspect.isclass(type)]

def vec_parse(str, clamp_min = None, clamp_max = None):
    """ Split a ',' delimeted string into a list of floats, possibly clamping them to min/max """
    split_string = str.split(",")
    min_clamper = (lambda x : x) if clamp_min == None else (lambda x : max(clamp_min, x))
    max_clamper = (lambda x : x) if clamp_max == None else (lambda x : min(clamp_max, x))
    try:
        values = map(float, split_string)
        values = map(min_clamper, values)
        values = map(max_clamper, values)
        return values
    except ValueError:
        return []
        
parsers = { float : lambda x : float(x),
            dx.Vector2 : lambda x : dx.Vector2(*vec_parse(x)), 
            dx.Vector3 : lambda x : dx.Vector3(*vec_parse(x)), 
            dx.Vector4 : lambda x : dx.Vector4(*vec_parse(x)), 
            dx.Color : lambda x : dx.Color(*vec_parse(x, clamp_min = 0, clamp_max = 1)), 
            }

class TreeItemBase:
    def __init__(self, name, parent):
        self.name = name
        self.parent_item = parent
        self.children = []
        self.column_count = { Root : 2, Effect : 1, Variable : 2 }
        if parent: 
            parent.appendChild(self)
            
    def find_child_by_name(self, child_name):
        def find_child_by_name_inner(cur, child_name):
            if cur.name == child_name:
                return cur
            for c in cur.children:
                result = find_child_by_name_inner(c, child_name)
                if result:
                    return result
            return None
        return find_child_by_name_inner(self, child_name)

    def appendChild(self, item):
        self.children.append(item)

    def child(self, row):
        return self.children[row]

    def childCount(self):
        return len(self.children)
        
    def is_variable(self):
        return False
    
    def is_material(self):
        return False
    
    def is_effect(self):
        return False

    def columnCount(self):
        cur_count = self.column_count.get(self.__class__, 0)
        if len(self.children) == 0:
            return cur_count
        return max( cur_count, max(map(lambda x : x.columnCount(), self.children)))

    def parent(self):
        return self.parent_item

    def row(self):
        if self.parent_item:
            return self.parent_item.children.index(self)
        return 0

class Root(TreeItemBase):
    def __init__(self):
        TreeItemBase.__init__(self, "root", None)

        
class Effect(TreeItemBase):
    def __init__(self, name, hlsl_str, parent):
        TreeItemBase.__init__(self, name, parent)
        self.materials = []
        self.defaults = []
        self.hlsl_str = hlsl_str
        
    def is_effect(self):
        return True
        
    def hlsl(self):
        return self.hlsl_str
        
    def data(self, column):
        return self.name if column == 0 else ""


class Material(TreeItemBase):
    def __init__(self, name, parent):
        TreeItemBase.__init__(self, name, parent)
        
    def data(self, column):
        return self.name if column == 0 else ""
    
    def is_material(self):
        return True

class Variable(TreeItemBase):
    def __init__(self, name, value, parent):
        TreeItemBase.__init__(self, name, parent)
        self.value = value
        self.is_hidden = False
        
    def is_variable(self):
        return True

    def data(self, column):
        if column == 0: 
            return self.name
        elif column == 1: 
            return self.value
        return ""

# colors from kuler.adobe.com
tin_toy = [ QColor(48, 44, 56), QColor(255, 214, 148), QColor(194, 99, 55), QColor(92, 87, 52), QColor(112, 1, 0) ]
melody = [ QColor(133, 100, 69), QColor(204, 127, 95), QColor(255, 145, 145), QColor(255, 226, 132), QColor(229, 192, 121) ]
wonderful = [ QColor(191, 6, 51), QColor(255, 72, 78), QColor(255, 146, 115), QColor(209, 208, 180), QColor(229, 236, 237)]
colors = [tin_toy, melody, wonderful]
cur_col = colors[0]

class VariableDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super(VariableDelegate, self).__init__(parent)
        
    def paint(self, painter, option, index):
        if index.internalPointer().is_variable():
            obj = index.internalPointer()
            if index.column() == 0:
                painter.fillRect(option.rect, QBrush(cur_col[0]))
                painter.setPen(cur_col[1])
                painter.drawText(option.rect, Qt.AlignCenter, obj.name)
            else:
                r, g, b = 1, 1, 1
                if obj.value.__class__ == dx.Color:
                    r, g, b = obj.value.r, obj.value.g, obj.value.b
                    painter.fillRect(option.rect, QBrush(QColor(255 * r, 255 * g, 255 * b)))
                    painter.setPen(QColor(128 + 127 * (1-r), 128 + 127 * (1-g), 128 + 127 * (1-b)))
                else:
                    painter.fillRect(option.rect, QBrush(cur_col[2]))
                    painter.setPen(cur_col[4])
                painter.drawText(option.rect, Qt.AlignCenter, str(obj.value))
        else:
            QStyledItemDelegate.paint(self, painter, option, index)
            
    def setModelData(self, editor, model, index):
        """ Update model after editing. """
        column = index.column()
        ptr = index.internalPointer()
        if isinstance(ptr, Variable):
            obj_type = model.data(index, Qt.EditRole).toPyObject().__class__
            parser = parsers.get(obj_type, None)
            if parser:
                try:
                    value = parser(editor.text())
                    if value:
                        model.setData(index, value, Qt.EditRole)
                except TypeError:
                    pass
        elif isinstance(ptr, Material):
            model.setData(index, str(editor.text()), Qt.EditRole)
            

            
class ConnectionModel(QAbstractTableModel):
    def __init__(self, parent=None):
        QAbstractTableModel.__init__(self, parent)
        self.connections = []
        self.headers = ["Mesh", "Material"]
        
    def data(self, index, role):
        if index.isValid() and role in [Qt.DisplayRole, Qt.EditRole]:
            col = index.column()
            if col == 0:
                return QVariant(self.connections[index.row()][0])
            elif col == 1:
                return QVariant(self.connections[index.row()][1].name)
        return QVariant()
    
    def data_by_row_col(self, row, col, role):
        new_index = self.index(row, col)
        return self.data(new_index, role)
    
    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()
    
    def rowCount(self, parent):
        return len(self.connections)
    
    def columnCount(self, parent):
        return 0 if len(self.connections) == 0 else len(self.connections[0])
    
    def flags(self, index):
        if not index.isValid(): 
            return Qt.ItemIsEnabled
        if index.column() == 0:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable
        return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
    
    def add_connections(self, cons):
        self.emit(SIGNAL("layoutAboutToBeChanged()"))
        self.connections = cons
        self.connections.sort(key=operator.itemgetter(0))
        self.emit(SIGNAL("layoutChanged()"))
    
    def sort(self, col, order):
        """Sort table by given column number."""
        self.emit(SIGNAL("layoutAboutToBeChanged()"))
        self.connections.sort(key=operator.itemgetter(col))
        if order == Qt.DescendingOrder:
            self.connections.reverse()
        self.emit(SIGNAL("layoutChanged()"))

    
class ConnectionDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super(ConnectionDelegate, self).__init__(parent)
        
    def get_mesh_name(self, model, index):
        return str(model.data_by_row_col(index.row(), 0, Qt.EditRole).toString())

    def get_material_name(self, model, index):
        return str(model.data_by_row_col(index.row(), 1, Qt.EditRole).toString())
    
    def material_changed(self, material_name):
        mesh_name = editor.selected_connection[0]
        material_name = str(material_name)
        material_manager.update_mesh(mesh_name, material_name)
        
    def createEditor(self, parent, option, index):
        col = index.column()
        if col != 1:
            return None
        
        material_name = editor.selected_connection[1]
        
        value_editor = QComboBox(parent)
        for i, material in enumerate(variable_manager.variable_model.get_items_by_type(Material)):
            value_editor.addItem(material.name)
            if material_name == material.name:
                value_editor.setCurrentIndex(i)
        self.connect(value_editor, SIGNAL("currentIndexChanged(const QString&)"), self.material_changed)
        return value_editor
    
            
    def setModelData(self, editor, model, index):
        """ Update model after editing. """
        col = index.column()
        idx = editor.currentIndex()
        mesh_name = self.get_mesh_name(model, index)
        material_name = str(editor.itemText(idx))
        material_manager.update_mesh(mesh_name, material_name)

def var_sorter(a, b) :
    sort_values = { Effect: 0, Variable : 1, Material : 2 }
    return sort_values[a.__class__] - sort_values[b.__class__]
        
class VariableModel(QAbstractItemModel):
    def __init__(self, parent=None):
        QAbstractItemModel.__init__(self, parent)
        self.root_item = Root()
        
    def sort_nodes(self):
        def sort_inner(node):
            node.children.sort(var_sorter)
            for c in node.children:
                sort_inner(c)
        self.emit( SIGNAL( "layoutAboutToBeChanged()" ) )                 
        sort_inner(self.root_item)
        self.emit( SIGNAL( "layoutChanged()" ) ) 
        
    def nodeFromIndex(self, index):
        return index.internalPointer() if index.isValid() else self.root_item

    def supportedDropActions(self):
        return Qt.MoveAction
    
    def find_material(self, material_name, parent_name = None):
        def find_material_inner(material_name, parent_name, cur):
            material_matches = cur.name == material_name
            if parent_name:
                if cur.parent_item and cur.parent_item.name == parent_name and material_matches:
                    return cur
            elif material_matches:
                    return cur
            for child in cur.children:
                result = find_material_inner(parent_name, material_name, child)
                if result:
                    return result
            return None
        return find_material_inner(material_name, parent_name, self.root_item)
        
    def find_item(self, name):
        def find_item_inner(name, cur):
            if cur.name == name:
                return cur
            for child in cur.children:
                result = find_item_inner(name, child)
                if result:
                    return result
            return None
        return find_item_inner(name, self.root_item)
        
    def get_items_by_type(self, item_type):
        def get_items_by_type_inner(cur, item_type, result):
            if cur.__class__ == item_type:
                result.append(cur)
            for c in cur.children:
                get_items_by_type_inner(c, item_type, result)
            return result
        return get_items_by_type_inner(self.root_item, item_type, [])

    def update_parent_by_name(self, child_name, parent_name):
        child_item = self.find_item(child_name)
        parent_item = self.find_item(parent_name)
        self.update_parent(child_item, parent_item)
    
    def update_parent(self, child_item, parent_item):
        """ Add the child to a new parent, removing it from the existing parent """
        if child_item.__class__ == Material and parent_item.__class__ == Effect:
            self.sync_variables(child_item, parent_item)
            
        inplace_filter(lambda x : x.name != child_item.name, child_item.parent_item.children)
        child_item.parent_item = parent_item
        parent_item.children.append(child_item)
        
    def sync_variables(self, material_item, effect_item):
        # Create variables with default values for the ones not present in the material
        for name, var_type, value in effect_item.defaults:
            if not material_item.find_child_by_name(name):
                Variable(name, value, material_item)
                
        # Mark the variables not used by the effect as hidden
        for c in material_item.children:
            var_name = c.name
            if not next((name for (name, _, _) in effect_item.defaults if name == var_name), None):
                print "not used " + c.name
                c.hidden = True
        
    def columnCount(self, parent):
        if parent.isValid():
            return parent.internalPointer().columnCount()
        else:
            return self.root_item.columnCount()

    def data(self, index, role):
        if not index.isValid():
            return QVariant()
        if role != Qt.DisplayRole and role != Qt.EditRole:
            return QVariant()
        item = index.internalPointer()
        d = item.data(index.column())
        return QVariant(d) if d else QVariant()
        
    def setData(self, index, value, role):
        wrapper = index.internalPointer()
        if role != Qt.EditRole:
            return False
        try:
            if isinstance(wrapper, Variable):
                wrapper.value = value
            elif isinstance(wrapper, Material):
                wrapper.name = value
                system.set_material_for_mesh("dummy", "dummy")
        except (ValueError, TypeError):
            print "Error converting: \"" + value.toString() + "\" to: " + str(wrapper.value.__class__)
        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)

        return True

    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        if index.internalPointer().is_variable():
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        if index.internalPointer().is_material():
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsEditable
        if index.internalPointer().is_effect():
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsDropEnabled
        return Qt.ItemIsEnabled | Qt.ItemIsSelectable
        

    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            headers = { 0: QVariant("Name"), 1: QVariant("Value") }
            return headers.get(section, QVariant())
        return QVariant()
        
    def mimeTypes(self):
        """ Return supported mime types """
        return QStringList(['application/x-ets-qt4-instance'])

    def mimeData(self, index):
        """ Encode node in correct type """
        return PyMimeData(self.nodeFromIndex(index[0]))
        
    def dropMimeData(self, mimedata, action, row, column, parentIndex):
        """ Handle dropping """
        print row, column
        if action == Qt.IgnoreAction:
            return True

        material_name = mimedata.instance().name
        effect_name = self.nodeFromIndex(parentIndex).name
        self.update_parent_by_name(material_name, effect_name)
        refresh_variable_view(editor.variable_view)
        self.sort_nodes()
        self.emit(SIGNAL("dataChanged(QModelIndex,QModelIndex)"), parentIndex, parentIndex)
        system.set_material_for_mesh("dummy", "dummy")
        return True


    def get_parent_item(self, parent):
        return self.root_item if not parent.isValid() else parent.internalPointer()

    def index(self, row, column, parent):
        if row < 0 or column < 0 or row >= self.rowCount(parent) or column >= self.columnCount(parent):
            return QModelIndex()

        parentItem = self.get_parent_item(parent)
        childItem = parentItem.child(row)
        
        return QModelIndex() if not childItem else self.createIndex(row, column, childItem)

    def parent(self, index):
        if not index.isValid():
            return QModelIndex()

        child_item = index.internalPointer()
        parent_item = child_item.parent_item

        if parent_item == self.root_item:
            return QModelIndex()

        return self.createIndex(parent_item.row(), 0, parent_item)

    def rowCount(self, parent):
        if parent.column() > 0:
            return 0
        return self.get_parent_item(parent).childCount()


class MaterialManager:
    def __init__(self):
        self.material_connections = []
        
    def get_textures(self):
        vars = variable_manager.variable_model.get_items_by_type(Variable)
        return [var.value for var in vars if var.name == texture_name]
        
    
    def update_mesh(self, mesh_name, material_name):
        """ Sets the new material for the mesh """
        material_item = variable_manager.variable_model.find_item(material_name)
        for i, v in enumerate(self.material_connections):
            if v[0] == mesh_name:
                self.material_connections[i] = (mesh_name, material_item)
                system.set_material_for_mesh(mesh_name, material_name)
                return
       
    def load_material_connections(self, filename):
        print "load_material_connections"
        cons = __import__(filename)
        for mesh_name, mat_name in cons.material_connections:
            mat_item = variable_manager.variable_model.find_item(mat_name)
            self.material_connections.append((mesh_name, mat_item))
        editor.connection_model.add_connections(self.material_connections)
        
        for effect_name, material_names in cons.effect_connections:
            for material_name in material_names:
                variable_manager.variable_model.update_parent_by_name(material_name, effect_name)
                
        variable_manager.variable_model.sort_nodes()
        refresh_variable_view(variable_manager.variable_view)

    def load_materials_from_file(self, filename):
        print "load_materials_from_file"
        variable_manager.load_materials(filename)
            
    def get_materials(self):
        """ Returns a list of the unique materials """
        unique_mats = set()
        for mesh, mat in self.material_connections:
            unique_mats.add(mat.name)
        return list(unique_mats)
        
    def get_material(self, name):
        """ Returns the material with the given name """
        mats = variable_manager.variable_model.get_items_by_type(Material)
        return next((mat for mat in mats if mat.name == name), None)
                
    def get_meshes_for_material(self, material_name):
        """ Returns a list of meshes that use the given material """
        return [mesh for (mesh, mat) in self.material_connections if mat.name == material_name]
        
    # todo, move to material        
    def material_has_texture(self, mat):
        for c in mat.children:
            if c.name == texture_name:
                return True
        return False
            
    # todo, move to material        
    def is_transparent(self, mat):
        trans_item = mat.find_child_by_name("transparency")
        return trans_item.value > 0



class EffectManager:
    def __init__(self):
        self.active_material = None
        self.active_effect = None
        
    def set_active_effect(self, effect):
        self.active_effect = effect
        self.active_material = None
        
    def get_effects(self):
        return [e.name for e in variable_manager.variable_model.get_items_by_type(Effect)]
        
    def get_materials_for_effect(self, effect_name):
        """ Returns a list of material names that use the given effect """
        effect_item = variable_manager.variable_model.find_item(effect_name)
        return [x.name for x in effect_item.children if isinstance(x, Material)]
    
    def set_variable(self, name, value):
        def find_variable(name):
            for var in self.active_effect.children:
                if var.name == name:
                    return var
            return None
            
        if self.active_effect and not self.active_material:
            var = find_variable(name)
            if var:
                var.value = value
            else:
                Variable(name, value, self.active_effect)
                refresh_variable_view(editor.variable_view)
        
    def apply_material(self, material):
        assert(self.active_effect)
        self.active_material = material
        variable_manager.set_active_material(material)
        material_item = variable_manager.variable_model.find_material(material.name, self.active_effect.name)
        if material_item:
            for c in material_item.children:
                # hack
                if c.name == texture_name:
                    system.set_resource(c.name, c.value)
                elif c.name != "transparency":
                    system.set_variable(c.name, c.value)
        
    def load_effect(self, filename):
        return variable_manager.load_effect_from_file(filename)
    
class VariableManager:
    def __init__(self, view, model):
        self.variable_view = view
        self.variable_model = model
        self.active_material = None
        
    def save(self):
        def save_to_json():
            f = open("var.dat", "wt")
            f.write(jsonpickle.encode(self.variable_model.root_item, unpicklable=False))
            f.write("\n")
            f.write(jsonpickle.encode(material_manager.material_connections, unpicklable=False))
        def save_to_binary():
            f = open("var.dat", "wb")
            pickle.dump(self.variable_model.root_item, f)
            pickle.dump(material_manager.material_connections, f)
        save_to_binary()
       
    def load(self):
        def load_from_json():
            f = open("var.dat", "rt")
            self.variable_model.root_item = jsonpickle.decode(f.readline())
            material_manager.material_connections = jsonpickle.decode(f.readline())
        def load_from_binary():
            f = open("var.dat", "rb")
            self.variable_model.root_item = pickle.load(f)
            material_manager.material_connections = pickle.load(f)
        load_from_binary()
        refresh_variable_view(self.variable_view)
        system.set_material_for_mesh("dummy", "dummy")

        
    def set_active_material(self, material):
        self.active_material = material
        
    
    def is_valid_effect(self, effect):
        required_methods = sorted(["apply_material", "hlsl", "variables"])
        return all([m in dir(effect) for m in required_methods])
        
    def load_effect_from_file(self, filename):
        print "loading effect from file: ", filename
        effect = __import__(filename)
        for class_name in classes_in_module(effect):
            c = getattr(effect, class_name)
            if self.is_valid_effect(c):
                if print_effects: 
                    print "found effect: " + class_name
                instance = c(system)
                e = Effect(class_name, instance.hlsl(), self.variable_model.root_item)
                e.defaults = instance.variables()
                for (name, type, default) in instance.variables():
                    Variable(name, default, e)
                refresh_variable_view(self.variable_view)
                return e
            else:
                print "Not a valid effect: " + class_name
                
    def load_materials(self, filename):
        print "loading materials from file: ", filename
        mats = __import__(filename)
        for class_name in classes_in_module(mats):
            # create an instance of material class
            instance = getattr(mats, class_name)()
            # we use the root as the parent until we load effect connections
            parent = self.variable_model.root_item
            material_item = Material(class_name, parent)
            for name, value in instance.values:
                Variable(name, value, material_item)

            if print_materials: 
                print "found material: " + class_name
        refresh_variable_view(self.variable_view)        

class EditorSettings:
    def __init__(self):
        self.editor_window_pos = (0,0)
        self.editor_window_size = (500, 500)
        self.editor_splitter_pos = 100
        self.engine_window_pos = (0,0)
        self.engine_window_size = (1024, 768)
        
class VariableView(QTreeView):
    def __init__(self):
        QTreeView.__init__(self)
        self.dragEnabled()
        self.acceptDrops()
        self.showDropIndicator()
        self.setDragDropMode(QAbstractItemView.InternalMove)


class EditorWindow(QMainWindow): 
    def __init__(self):
        QMainWindow.__init__(self)
        self.selected_connection = []
        self.variable_model = VariableModel()
        self.variable_view = VariableView()
        self.variable_view.setModel(self.variable_model)
        self.variable_view.setItemDelegate(VariableDelegate(app))
        
        self.connection_model = ConnectionModel()
        self.connection_view = QTableView()
        self.connection_view.setModel(self.connection_model)
        self.connection_view.setShowGrid(True)
        self.connection_view.setItemDelegate(ConnectionDelegate(app))
        self.connection_view.setSortingEnabled(True)
        self.connection_view.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.connection_view.setSelectionMode(QAbstractItemView.SingleSelection)
        self.connection_view.setEditTriggers(QAbstractItemView.DoubleClicked | QAbstractItemView.SelectedClicked)
        self.connection_selection_model = QItemSelectionModel(self.connection_model, self.connection_view)
        self.connection_view.setSelectionModel(self.connection_selection_model)
        self.connect(self.connection_selection_model, \
                     SIGNAL("selectionChanged(const QItemSelection&, const QItemSelection&)"), self.connection_selection_changed)

        layout = QVBoxLayout()
        
        button_layout = QHBoxLayout()
        self.load_button = QPushButton("load")
        self.save_button = QPushButton("save")
        self.reset_button = QPushButton("reset")
        button_layout.addWidget(self.load_button)
        button_layout.addWidget(self.save_button)
        button_layout.addWidget(self.reset_button)

        tab_widget = QTabWidget()
        variable_tab = QWidget()
        variable_tab_layout = QVBoxLayout()
        variable_tab_layout.addWidget(self.variable_view)

        variable_button_layout = QHBoxLayout()
        self.var_clone_button = QPushButton("clone")
        self.var_add_button = QPushButton("add")
        self.var_delete_button = QPushButton("delete")
        variable_button_layout.addWidget(self.var_clone_button)
        variable_button_layout.addWidget(self.var_add_button)
        variable_button_layout.addWidget(self.var_delete_button)
        variable_tab_layout.addLayout(variable_button_layout)
       
        variable_tab.setLayout(variable_tab_layout)
        connection_tab = QWidget()
        connection_tab_layout = QVBoxLayout()
        connection_tab_layout.addWidget(self.connection_view)
        connection_tab.setLayout(connection_tab_layout)

        tab_widget.addTab(variable_tab, "Variables")
        tab_widget.addTab(connection_tab, "Connections")
        layout.addWidget(tab_widget)
        
        #layout.addWidget(self.variable_view)
        layout.addLayout(button_layout)

        exit = QAction('Exit', self)
        exit.setShortcut('Ctrl+Q')
        exit.setStatusTip('Exit application')
        self.connect(exit, SIGNAL('triggered()'), self.quit_pressed)

        self.statusBar()

        menubar = self.menuBar()
        file = menubar.addMenu('&File')
        file.addAction(exit)
        
        central_widget = QWidget()
        central_widget.setLayout(layout)
        self.setCentralWidget(central_widget)
        
        self.resize(500, 768)
        self.move(0, 0)
        
    def connection_selection_changed(self, cur, prev):
        self.selected_connection = map(lambda x : str(x.data().toString()), cur.indexes())
        
    def quit_pressed(self):
        #settings = QSettings()
        #settings.setValue('Geometry', QVariant(self.saveGeometry()))
        qApp.quit()
       
    
class MainWindow(QWidget):
    def __init__(self):
        QWidget.__init__(self)
        self.canvas = QWidget()

        layout = QVBoxLayout()
        layout.addWidget(self.canvas)
        self.setLayout(layout)

        self.resize(1024, 768)
        self.setMinimumSize(800, 600)
        self.move(520, 0)
        self.setMouseTracking(True)
        
        self.timer = QTimer(self)
        self.connect(self.timer, SIGNAL("timeout()"), self.update)
        self.timer.start(1000 / 60)
        self.first_move = True
        self.last_x = 0
        self.last_y = 0
        
    def update(self):
        system.render()
        
    def event(self, event):
        if event.type() == QEvent.KeyPress:
            system.key_down(int(event.key()))
            return True
        elif event.type() == QEvent.KeyRelease:
            system.key_up(int(event.key()))
            return True
        elif event.type() == QEvent.MouseButtonPress:
            system.mouse_click(event.button() == 1, event.button() == 2);
            return True
        elif event.type() == QEvent.MouseButtonRelease:
            system.mouse_click(False, False);
            return True
        elif event.type() == QEvent.MouseMove:
            if not self.first_move:
                system.mouse_move(event.x(), self.last_x, event.y(), self.last_y)
            else:
                self.first_move = False
            self.last_x = event.x()
            self.last_y = event.y()
            return True
        return QWidget.event(self, event)
    
def reset_pressed():
    system.reset()
    
def clone_pressed():
    print "clone"
    
def add_pressed():
    new_material = Material("new", variable_manager.variable_model.root_item)
    refresh_variable_view(variable_manager.variable_view)
    pass
    
def changed(cur, prev):
    pass

if __name__ == "__main__":
    print "Starting system"
    
    app = QApplication([""])
    system = system_ext.System()
    main_window = MainWindow()
    main_window.show()
    
    editor = EditorWindow()
    editor.show()
   
    variable_manager = VariableManager(editor.variable_view, editor.variable_model)
    effect_manager = EffectManager()
    material_manager = MaterialManager()
    
    editor.connect(editor.load_button, SIGNAL("pressed()"), variable_manager.load)
    editor.connect(editor.save_button, SIGNAL("pressed()"), variable_manager.save)
    editor.connect(editor.reset_button, SIGNAL("pressed()"), reset_pressed)
    
    editor.connect(editor.var_clone_button, SIGNAL("pressed()"), clone_pressed)
    editor.connect(editor.var_add_button, SIGNAL("pressed()"), add_pressed)
    
    system.set_effect_manager(effect_manager)
    system.init_directx(int(main_window.canvas.winId()))

    renderer = redux_ext.DefaultRenderer(system, effect_manager, material_manager)
    #renderer.load_scene("data/scenes/nissan.rdx")
    renderer.load_scene("data/scenes/gununit.rdx")
    #renderer.load_scene("data/scenes/rum.rdx")
    
    app.exec_()
