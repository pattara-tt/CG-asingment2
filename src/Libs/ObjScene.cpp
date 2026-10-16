#include "ObjScene.h"
#include "stb_image.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cfloat>

namespace {
std::string trim(const std::string& s) { auto a=s.find_first_not_of(" \t\r\n"); if(a==std::string::npos)return ""; auto b=s.find_last_not_of(" \t\r\n");return s.substr(a,b-a+1); }
std::string filename(const std::string& s) { auto p=s.find_last_of("/\\");return p==std::string::npos?s:s.substr(p+1); }
int fix(int i,int n) {return i>0?i-1:(i<0?n+i:-1);}
struct Trip {int p=-1,t=-1,n=-1;bool operator==(const Trip& b)const{return p==b.p&&t==b.t&&n==b.n;}};
struct TH {size_t operator()(Trip a)const {return size_t(a.p+1)*73856093u ^ size_t(a.t+1)*19349663u ^ size_t(a.n+1)*83492791u;}};
Trip parse(std::string s,int p,int t,int n) {
 Trip x; int v[3]{0,0,0};size_t st=0;
 for(int k=0;k<3&&st<=s.size();++k){auto end=s.find('/',st);auto a=s.substr(st,end==std::string::npos?std::string::npos:end-st);try{if(!a.empty())v[k]=std::stoi(a);}catch(...){} if(end==std::string::npos)break;st=end+1;}
 x.p=fix(v[0],p);x.t=fix(v[1],t);x.n=fix(v[2],n);return x;
}
GLuint loadTexture(const std::filesystem::path& p) {
 int w=0,h=0,c=0;stbi_set_flip_vertically_on_load(true);
 unsigned char* data=stbi_load(p.string().c_str(),&w,&h,&c,4);
 if(!data){std::cerr<<"Texture not found: "<<p<<"\n";return 0;}
 GLuint tex=0;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,data);glGenerateMipmap(GL_TEXTURE_2D);stbi_image_free(data);return tex;
}
struct Build {std::vector<VertexPNU> verts;std::vector<unsigned> inds;std::unordered_map<Trip,unsigned,TH> seen;};
}
ObjScene::~ObjScene(){std::vector<GLuint> ids;for(auto& m:materials){if(m.second.texture)ids.push_back(m.second.texture);if(m.second.opacityTexture)ids.push_back(m.second.opacityTexture);}std::sort(ids.begin(),ids.end());ids.erase(std::unique(ids.begin(),ids.end()),ids.end());if(!ids.empty())glDeleteTextures((GLsizei)ids.size(),ids.data());}
bool ObjScene::Load(const std::filesystem::path& objPath,const std::filesystem::path& texDir){
 std::ifstream in(objPath);if(!in){std::cerr<<"OBJ missing: "<<objPath<<"\n";return false;}
 // MTL images are resolved by filename, never by the absolute Windows path saved by Blender.
 std::ifstream matIn(objPath.parent_path()/(objPath.stem().string()+".mtl"));
 std::string line,current;
 while(std::getline(matIn,line)){
  std::istringstream ss(line);std::string tag;ss>>tag;
  if(tag=="newmtl"){std::getline(ss,current);current=trim(current);materials[current];}
  else if(!current.empty()){
   auto& m=materials[current];if(tag=="Kd")ss>>m.color.r>>m.color.g>>m.color.b;
   else if(tag=="d")ss>>m.alpha;
   else if(tag=="Tr"){float transparency=0;ss>>transparency;m.alpha=1-transparency;}
   else if(tag=="map_Kd"||tag=="map_d"){std::string v;std::getline(ss,v);v=filename(trim(v));if(tag=="map_Kd")m.image=v;else m.opacityImage=v;}
  }
 }
 std::map<std::string,Build> builders;
 std::vector<glm::vec3> P,N;std::vector<glm::vec2> UV;
 glm::vec3 lo(FLT_MAX),hi(-FLT_MAX);std::string material="Material",object;
 size_t skipped=0,faces=0;
 while(std::getline(in,line)){
  if(line.empty()||line[0]=='#')continue;
  std::istringstream ss(line);std::string tag;ss>>tag;
  if(tag=="o"){std::getline(ss,object);object=trim(object);}
  else if(tag=="usemtl"){std::getline(ss,material);material=trim(material);}
  else if(tag=="v"){glm::vec3 p;ss>>p.x>>p.y>>p.z;P.push_back(p);}
  else if(tag=="vt"){glm::vec2 uv;ss>>uv.x>>uv.y;UV.push_back(uv);}
  else if(tag=="vn"){glm::vec3 n;ss>>n.x>>n.y>>n.z;N.push_back(n);}
  else if(tag=="f"){
   // The export includes an unwanted Blender default Cube (-1..1) which blocks the diorama.
   // Keep it in source OBJ but do not render it. Re-export from Blender without Cube for a permanent fix.
   if(object=="Cube"){++skipped;continue;}
   auto& b=builders[material];std::vector<Trip> v;std::string token;
   while(ss>>token){Trip t=parse(token,(int)P.size(),(int)UV.size(),(int)N.size());if(t.p>=0&&t.p<(int)P.size())v.push_back(t);}
   if(v.size()<3)continue;
   auto add=[&](Trip t){auto it=b.seen.find(t);if(it!=b.seen.end()){b.inds.push_back(it->second);return;}
     VertexPNU x{};x.position=P[t.p];if(t.t>=0&&t.t<(int)UV.size())x.uv=UV[t.t];if(t.n>=0&&t.n<(int)N.size())x.normal=N[t.n];
     unsigned id=(unsigned)b.verts.size();b.verts.push_back(x);b.seen.emplace(t,id);b.inds.push_back(id);
     lo=glm::min(lo,x.position);hi=glm::max(hi,x.position);
   };
   for(size_t i=1;i+1<v.size();i++){add(v[0]);add(v[i]);add(v[i+1]);++faces;}
  }
 }
 if(!faces)return false;
 center=(lo+hi)*0.5f;radius=glm::length(hi-lo)*0.5f;
 std::map<std::string,GLuint> cache;
 for(auto& kv:materials){auto& m=kv.second;
   auto get=[&](const std::string& fn)->GLuint{if(fn.empty())return 0;auto it=cache.find(fn);if(it!=cache.end())return it->second;GLuint tex=loadTexture(texDir/fn);cache[fn]=tex;return tex;};
   m.texture=get(m.image);m.opacityTexture=get(m.opacityImage);
 }
 // Cache stores one shared GL texture per filename. Each texture is deleted exactly once in destructor.
 for(auto& kv:builders){auto mesh=std::make_unique<Mesh>();mesh->CreateMesh(kv.second.verts,kv.second.inds);parts.push_back({std::move(mesh),kv.first});}
 std::cout<<"Loaded "<<faces<<" triangles, "<<parts.size()<<" materials, "<<cache.size()<<" texture files. Skipped "<<skipped<<" default Cube faces. Center="<<center.x<<","<<center.y<<","<<center.z<<" radius="<<radius<<"\n";
 return true;
}
void ObjScene::Render(GLint col,GLint use,GLint alpha,GLint opacity,GLint useOpacity){
 glActiveTexture(GL_TEXTURE0);
 for(auto& part:parts){auto it=materials.find(part.material);SceneMaterial fallback;auto& m=it==materials.end()?fallback:it->second;
  glUniform3fv(col,1,&m.color.x);glUniform1f(alpha,m.alpha);glUniform1i(use,m.texture?1:0);
  glBindTexture(GL_TEXTURE_2D,m.texture);
  glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,m.opacityTexture);glUniform1i(useOpacity,m.opacityTexture?1:0);glUniform1i(opacity,1);glActiveTexture(GL_TEXTURE0);
  part.mesh->RenderMesh();
 }
}
