#ifndef REDUX_TYPES_HPP
#define REDUX_TYPES_HPP

class Mesh;
class Geometry;
class Light;
class Camera;
class Handle;
//class Material;
class AnimationNode;
class AnimationManager;

#ifdef STANDALONE
//typedef Handle EffectObj;
//typedef Handle MaterialObj;
#else
typedef boost::python::object EffectObj;
typedef boost::python::object MaterialObj;
#endif

typedef CComPtr<ID3D10InputLayout> InputLayoutPtr;

typedef boost::shared_ptr<Geometry> GeometryPtr;
//typedef boost::shared_ptr<AnimationNode> AnimationNodePtr;
//typedef boost::shared_ptr<AnimationManager> AnimationManagerPtr;
typedef boost::shared_ptr<Mesh> MeshSPtr;
typedef boost::shared_ptr<Light> LightPtr;
typedef boost::shared_ptr<Camera> CameraPtr;
typedef boost::shared_ptr<AnimationNode> AnimationNodeSPtr;

typedef std::string MeshName;
typedef std::string MaterialName;
typedef std::string EffectName;
typedef std::string NodeName;
typedef uint32_t NodeId;

#endif // #ifndef REDUX_TYPES_HPP
