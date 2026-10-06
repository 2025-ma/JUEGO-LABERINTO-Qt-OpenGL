#include "assimp/Mesh.h"

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLFunctions>


Mesh::Mesh (QOpenGLFunctions_3_3_Core *aFun, GLuint vertexLoc, GLuint normalLoc, GLuint texUVLoc ,  GLuint diffuseLoc, GLuint specularLoc, GLuint ambientLoc, GLuint shininessLoc){
	this->fun = aFun;
	this->vertexLoc = vertexLoc;
	this->normalLoc = normalLoc;	
	this->texUVLoc = texUVLoc;		
    this->diffuseLoc = diffuseLoc;
    this->specularLoc = specularLoc;
    this->ambientLoc = ambientLoc;
    this->shininessLoc = shininessLoc;


    boundingBoxMax = glm::vec3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
    boundingBoxMin = glm::vec3( FLT_MAX, FLT_MAX, FLT_MAX );
}


Mesh::MeshEntry::MeshEntry()
{
    vao = INVALID_OGL_VALUE;
    VB = INVALID_OGL_VALUE;
    IB = INVALID_OGL_VALUE;
    NumIndices  = 0;
    MaterialIndex = INVALID_MATERIAL;
};

Mesh::MeshEntry::~MeshEntry()
{
    if (parent != NULL && VB != INVALID_OGL_VALUE)
    {
        parent->fun->glDeleteBuffers(1, &VB);
    }

    if (parent != NULL && IB != INVALID_OGL_VALUE)
    {
        parent->fun->glDeleteBuffers(1, &IB);
    }
}

void Mesh::MeshEntry::Init(const std::vector<MVertex>& Vertices,
                          const std::vector<unsigned int>& Indices, Mesh* pParent)
{
    NumIndices = Indices.size();
    parent = pParent;

    parent->fun->glGenVertexArrays(1, &vao);
    parent->fun->glBindVertexArray(vao);

    parent->fun->glGenBuffers(1, &VB);
    parent->fun->glBindBuffer(GL_ARRAY_BUFFER, VB);
    parent->fun->glBufferData(GL_ARRAY_BUFFER, sizeof(MVertex) * Vertices.size(), &Vertices[0], GL_STATIC_DRAW);
    

    parent->fun->glVertexAttribPointer(parent->vertexLoc, 3, GL_FLOAT, GL_FALSE, sizeof(MVertex), 0);
    parent->fun->glVertexAttribPointer(parent->texUVLoc, 2, GL_FLOAT, GL_FALSE, sizeof(MVertex), (const GLvoid*)12);
    parent->fun->glVertexAttribPointer(parent->normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(MVertex), (const GLvoid*)20);
    parent->fun->glVertexAttribPointer(parent->diffuseLoc, 3, GL_FLOAT, GL_FALSE, sizeof(MVertex), (const GLvoid*)32);
    parent->fun->glVertexAttribPointer(parent->specularLoc, 3, GL_FLOAT, GL_FALSE, sizeof(MVertex), (const GLvoid*)44);
    parent->fun->glVertexAttribPointer(parent->ambientLoc, 3, GL_FLOAT, GL_FALSE, sizeof(MVertex), (const GLvoid*) 56);
    parent->fun->glVertexAttribPointer(parent->shininessLoc, 1, GL_FLOAT, GL_FALSE, sizeof(MVertex), (const GLvoid*)68);


    parent->fun->glEnableVertexAttribArray(parent->vertexLoc);
    parent->fun->glEnableVertexAttribArray(parent->texUVLoc);
    parent->fun->glEnableVertexAttribArray(parent->normalLoc);
    parent->fun->glEnableVertexAttribArray(parent->diffuseLoc);
    parent->fun->glEnableVertexAttribArray(parent->specularLoc);
    parent->fun->glEnableVertexAttribArray(parent->ambientLoc);
    parent->fun->glEnableVertexAttribArray(parent->shininessLoc);

    parent->fun->glEnableVertexAttribArray(0);
    
    parent->fun->glGenBuffers(1, &IB);
    parent->fun->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
    parent->fun->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * NumIndices, &Indices[0], GL_STATIC_DRAW);

    
    parent->fun->glBindVertexArray(0);    
}



Mesh::~Mesh()
{
    Clear();
}


void Mesh::Clear()
{
    for (unsigned int i = 0 ; i < m_Textures_diff.size() ; i++) {        
        SAFE_DELETE(m_Textures_diff[i]);
    }
    for (unsigned int i = 0 ; i < m_Textures_norm.size() ; i++) {        
        SAFE_DELETE(m_Textures_norm[i]);
    }
}


bool Mesh::LoadMesh(const std::string& Filename)
{
    // Release the previously loaded mesh (if it exists)
    Clear();

    bool Ret = false;
    Assimp::Importer Importer;

    const aiScene* pScene = Importer.ReadFile(Filename.c_str(), ASSIMP_LOAD_FLAGS);

    if (pScene) {
        Ret = InitFromScene(pScene, Filename);
    }
    else {
        printf("Error parsing '%s': '%s'\n", Filename.c_str(), Importer.GetErrorString());
    }




    return Ret;
}

bool Mesh::InitFromScene(const aiScene* pScene, const std::string& Filename)
{
    m_Entries.resize(pScene->mNumMeshes);
    m_Textures_diff.resize(pScene->mNumMaterials);
    m_Textures_norm.resize(pScene->mNumMaterials);


    // Initialize the meshes in the scene one by one
    for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        InitMesh(i, paiMesh, pScene);
    }

    return InitMaterials(pScene, Filename);
}

void Mesh::InitMesh(unsigned int Index, const aiMesh* paiMesh, const aiScene* pScene)
{
    m_Entries[Index].MaterialIndex = paiMesh->mMaterialIndex;

    aiMaterial* mat = pScene->mMaterials[paiMesh->mMaterialIndex];

    std::vector<MVertex> Vertices;
    std::vector<unsigned int> Indices;

    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    aiColor4D ambient(0.1,0.1,0.1,0.1);
    aiColor4D diffuse(1.0,1.0,1.0,1.0);
    aiColor4D specular(0.4,0.4,0.4,0.4);
    

    aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE,    &diffuse);
    aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR,    &specular);
    aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT,    &ambient);

    float shininess = 10.f; // default value, remains unmodified if we fail.
    aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, (float*)&shininess);

    for (unsigned int i = 0 ; i < paiMesh->mNumVertices ; i++) {
        const aiVector3D* pPos      = &(paiMesh->mVertices[i]);
        const aiVector3D* pNormal   = &(paiMesh->mNormals[i]);
        const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

        glm::vec3 point = glm::vec3(pPos->x, pPos->y, pPos->z);
        boundingBoxMax = glm::max(boundingBoxMax, point);
        boundingBoxMin = glm::min(boundingBoxMin, point);

        MVertex v(point,
                 glm::vec2(pTexCoord->x, pTexCoord->y),
                 glm::vec3(pNormal->x, pNormal->y, pNormal->z),
                 glm::vec3(diffuse.r, diffuse.g, diffuse.b),
                 glm::vec3(specular.r, specular.g, specular.b),
                 glm::vec3(ambient.r, ambient.g, ambient.b),
                shininess
                );

        Vertices.push_back(v);
    }

    for (unsigned int i = 0 ; i < paiMesh->mNumFaces ; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        assert(Face.mNumIndices == 3);
        Indices.push_back(Face.mIndices[0]);
        Indices.push_back(Face.mIndices[1]);
        Indices.push_back(Face.mIndices[2]);
    }

    m_Entries[Index].Init(Vertices, Indices, this);
}

bool Mesh::InitMaterials(const aiScene* pScene, const std::string& Filename)
{
    // Extract the directory part from the file name
    std::string::size_type SlashIndex = Filename.find_last_of("/");
    std::string Dir;

    if (SlashIndex == std::string::npos) {
        Dir = ".";
    }
    else if (SlashIndex == 0) {
        Dir = "/";
    }
    else {
        Dir = Filename.substr(0, SlashIndex);
    }

    bool Ret = true;

    // Initialize the materials
    for (unsigned int i = 0 ; i < pScene->mNumMaterials ; i++) {
        const aiMaterial* pMaterial = pScene->mMaterials[i];

        m_Textures_diff[i] = NULL;
        m_Textures_norm[i] = NULL;
        
        if (pMaterial->GetTextureCount(aiTextureType_NORMALS) > 0) {            
            aiString Path;

            if (pMaterial->GetTexture(aiTextureType_NORMALS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                std::string FullPath = Dir + "/" + Path.data;
                m_Textures_norm[i] = new Texture(GL_TEXTURE_2D, FullPath.c_str());

                if (!m_Textures_norm[i]->Load()) {
                    printf("Error loading texture '%s'\n", FullPath.c_str());
                    delete m_Textures_norm[i];
                    m_Textures_norm[i] = NULL;
                    Ret = false;
                }
                else {
                    printf("Loaded texture '%s'\n", FullPath.c_str());
                }
            }
        }


        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString Path;

            if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                std::string FullPath = Dir + "/" + Path.data;
                m_Textures_diff[i] = new Texture(GL_TEXTURE_2D, FullPath.c_str());

                if (!m_Textures_diff[i]->Load()) {
                    printf("Error loading texture '%s'\n", FullPath.c_str());
                    delete m_Textures_diff[i];
                    m_Textures_diff[i] = NULL;
                    Ret = false;
                }
                else {
                    printf("Loaded texture '%s'\n", FullPath.c_str());
                }
            }
        }

        // Load a white texture in case the model does not include its own texture
        if (!m_Textures_diff[i]) {
            m_Textures_diff[i] = new Texture(GL_TEXTURE_2D, "../Content/white.png");

            Ret = m_Textures_diff[i]->Load();
        }
    }

    return Ret;
}

void Mesh::Render()
{
    for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {   
        this->fun->glBindVertexArray(m_Entries[i].vao);
        
        const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

        if (MaterialIndex < m_Textures_diff.size() && m_Textures_diff[MaterialIndex]) {
            m_Textures_diff[MaterialIndex]->Bind(GL_TEXTURE0);
            if( m_Textures_norm[MaterialIndex]){
                m_Textures_norm[MaterialIndex]->Bind(GL_TEXTURE1);
            } 
        }
        
        this->fun->glDrawElements(GL_TRIANGLES, m_Entries[i].NumIndices, GL_UNSIGNED_INT, 0);
        this->fun->glBindVertexArray(0);
   }

	
	

}
 
