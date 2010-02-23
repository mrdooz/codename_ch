import dx_ext as dx

class blinn_effect():
    def __init__(self, system):
        self.system = system
        self.filename = "effects\\blinn_effect.fx"
        self.name = "blinn_effect"
        
    def variables(self):
        return [
                #("projection",  dx.Matrix,  None), 
                #("view",  dx.Matrix,  None), 
                #("world",  dx.Matrix,  None), 
                #("eye_pos",  dx.Vector3,  dx.Vector3(0,0,0)), 
                ("transparency", float, 0.0),
                ("ambient_color",  dx.Color,  dx.Color(0.5,  0.5,  0.5,  1)), 
                ("diffuse_color",  dx.Color,  dx.Color(1.0,  0,  1.0,  1)), 
                ("emissive_color",  dx.Color,  dx.Color(0.5,  0.5,  0.5,  1)), 
                ]
    
    def apply_material(self, material):
        self.system.set_variable("ambient_color", material.ambient_color())
        self.system.set_variable("diffuse_color", material.diffuse_color())
        self.system.set_variable("emissive_color", material.emissive_color())
        
    def hlsl(self): 
        try:
            return "\n".join(open(self.filename).readlines())
        except IOError:
            print "Error loading " + self.filename
            return ""
    
