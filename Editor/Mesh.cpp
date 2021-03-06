#include "Mesh.h"
#include "FileIO.h"
#include "deps/Assimp/include/assimp/Importer.hpp"
#include "deps/Assimp/include/assimp/postprocess.h"
#include "deps/Assimp/include/assimp/cimport.h"

namespace UltraEd
{
	CMesh::CMesh(const char *filePath)
	{
		Assimp::Importer importer;
		m_info = CFileIO::Import(filePath);
		const aiScene *scene = importer.ReadFile(m_info.path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
			aiProcess_OptimizeMeshes);

		if (scene)
		{
			Process(scene->mRootNode, scene);
		}
	}

	void CMesh::Process(aiNode *node, const aiScene *scene)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			InsertVerts(node->mTransformation, mesh);
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			Process(node->mChildren[i], scene);
		}
	}

	void CMesh::InsertVerts(aiMatrix4x4 transform, aiMesh *mesh)
	{
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				Vertex vertex;

				// Apply the current transform to the mesh
				// so it renders in the correct local location.
				aiVector3D transformedVertex = mesh->mVertices[face.mIndices[j]];
				aiTransformVecByMatrix4(&transformedVertex, &transform);

				vertex.position.x = transformedVertex.x;
				vertex.position.y = transformedVertex.y;
				vertex.position.z = transformedVertex.z;

				aiVector3D normal = mesh->mNormals[face.mIndices[j]];
				vertex.normal.x = normal.x;
				vertex.normal.y = normal.y;
				vertex.normal.z = normal.z;

				if (mesh->HasTextureCoords(0))
				{
					vertex.tu = mesh->mTextureCoords[0][face.mIndices[j]].x;
					vertex.tv = mesh->mTextureCoords[0][face.mIndices[j]].y;
				}

				m_vertices.push_back(vertex);
			}
		}
	}
}
