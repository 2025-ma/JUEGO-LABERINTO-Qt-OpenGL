#ifndef MESH_H
#define MESH_H

#define GLM_ENABLE_EXPERIMENTAL
//using namespace std;

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <string>
#include <map>
#include <vector>



#include "include/assimp/Importer.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>



#include "ogldev_util.h"
#include "ogldev_texture.h"



struct MVertex
{
    glm::vec3 m_pos;
    glm::vec2 m_tex;
    glm::vec3 m_normal;
    glm::vec3 m_diffuse;
    glm::vec3 m_specular;
    glm::vec3 m_ambient;
    float m_shininess;
 
    MVertex() {}

    MVertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal, glm::vec3 diffuse, glm::vec3 specular, glm::vec3 ambient, float shininess)
    {
        m_pos    = pos;
        m_tex    = tex;
        m_normal = normal;
        m_diffuse = diffuse;
        m_specular = specular;
        m_ambient = ambient;
        m_shininess = shininess;
    }

    MVertex(const glm::vec3& pos, const glm::vec2& tex)
    {
        m_pos    = pos;
        m_tex    = tex;
        m_normal = glm::vec3(0.0f, 0.0f, 0.0f);
    }
};



class Mesh 
{
 QOpenGLFunctions_3_3_Core *fun;
  
public:
    Mesh(QOpenGLFunctions_3_3_Core *fun , GLuint vertexLoc, GLuint normalLoc, GLuint texUVLoc,  GLuint diffuseLoc, GLuint specularLoc, GLuint ambientLoc, GLuint shininessLoc);

    ~Mesh();

    bool LoadMesh(const std::string& Filename);

    void Render();

    glm::vec3 GetBBMax(){
        return boundingBoxMax;
    }

    glm::vec3 GetBBMin(){
        return boundingBoxMin;
    }
    glm::vec3 GetBBSize(){
        return boundingBoxMax-boundingBoxMin;
    }    


private:
    bool InitFromScene(const aiScene* pScene, const std::string& Filename);
    void InitMesh(unsigned int Index, const aiMesh* paiMesh, const aiScene* pScene);
    bool InitMaterials(const aiScene* pScene, const std::string& Filename);
    void Clear();

#define INVALID_MATERIAL 0xFFFFFFFF

    struct MeshEntry {
	Mesh* parent = NULL;    

        MeshEntry();

        ~MeshEntry();

        void Init(const std::vector<MVertex>& Vertices,
                  const std::vector<unsigned int>& Indices, Mesh* pParent);

        GLuint vao;
        GLuint VB;
        GLuint IB;

        unsigned int NumIndices;
        unsigned int MaterialIndex;
    };

    std::vector<MeshEntry> m_Entries;
    std::vector<Texture*> m_Textures_diff;
    std::vector<Texture*> m_Textures_norm;
    
    GLuint vertexLoc, normalLoc, texUVLoc, diffuseLoc, specularLoc, ambientLoc, shininessLoc;
    glm::vec3 boundingBoxMax, boundingBoxMin;

    
};


#endif  /* MESH_H */



