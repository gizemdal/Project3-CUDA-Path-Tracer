#include <iostream>
#include "scene.h"
#include <cstring>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/string_cast.hpp>

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

static int num_triangles = 0;
static int num_meshes = 0;

Scene::Scene(string filename) {
    cout << "Reading scene from " << filename << " ..." << endl;
    cout << " " << endl;
    char* fname = (char*)filename.c_str();
    fp_in.open(fname);
    if (!fp_in.is_open()) {
        cout << "Error reading from file - aborting!" << endl;
        throw;
    }
    while (fp_in.good()) {
        string line;
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            if (strcmp(tokens[0].c_str(), "MATERIAL") == 0) {
                loadMaterial(tokens[1]);
                cout << " " << endl;
            } else if (strcmp(tokens[0].c_str(), "OBJECT") == 0) {
                loadGeom(tokens[1]);
                cout << " " << endl;
            } else if (strcmp(tokens[0].c_str(), "CAMERA") == 0) {
                loadCamera();
                cout << " " << endl;
            }
        }
    }
}

int Scene::loadGeom(string objectid) {
    int id = atoi(objectid.c_str()) + num_triangles - num_meshes;
    if (id != geoms.size()) {
        cout << "Id: " << id << endl;
        cout << "Geoms: " << geoms.size() << endl;
        cout << "Triangles: " << num_triangles << endl;
        cout << "ERROR: OBJECT ID does not match expected number of geoms" << endl;
        return -1;
    } else {
        cout << "Loading Geom " << id << "..." << endl;
        Geom newGeom;
        string line;
        std::vector<Geom> triangles; // for arbitrary mesh only

        //load object type
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            if (strcmp(line.c_str(), "sphere") == 0) {
                cout << "Creating new sphere..." << endl;
                newGeom.type = SPHERE;
            } else if (strcmp(line.c_str(), "cube") == 0) {
                cout << "Creating new cube..." << endl;
                newGeom.type = CUBE;
            } else if (strcmp(line.c_str(), "mesh") == 0) {
                cout << "Creating new mesh..." << endl;
                newGeom.type = TRIANGLE;
                // read GLTF file
                string filename;
                utilityCore::safeGetline(fp_in, line);
                if (!line.empty() && fp_in.good()) {
                    tinygltf::Model model;
                    tinygltf::TinyGLTF loader;
                    std::string err;
                    std::string warn;
                    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, line.c_str());
                    if (!warn.empty()) {
                        printf("Warn: %s\n", warn.c_str());
                        return -1;
                    }
                    if (!err.empty()) {
                        printf("Err: %s\n", err.c_str());
                        return -1;
                    }
                    if (!ret) {
                        printf("Failed to parse glTF\n");
                        return -1;
                    }
                    for (tinygltf::Mesh mesh : model.meshes) {
                        for (tinygltf::Primitive prim : mesh.primitives) {
                            // Get the accessors and buffer views
                            const tinygltf::Accessor& accessorPos = model.accessors[prim.attributes["POSITION"]];
                            const tinygltf::BufferView& bufferViewPos = model.bufferViews[accessorPos.bufferView];

                            const tinygltf::Accessor& accessorNorm = model.accessors[prim.attributes["NORMAL"]];
                            const tinygltf::BufferView& bufferViewNorm = model.bufferViews[accessorNorm.bufferView];

                            const tinygltf::Buffer& bufferPos = model.buffers[bufferViewPos.buffer];
                            const tinygltf::Buffer& bufferNorm = model.buffers[bufferViewNorm.buffer];

                            const float* positions = reinterpret_cast<const float*>(&bufferPos.data[bufferViewPos.byteOffset + accessorPos.byteOffset]);
                            const float* normals = reinterpret_cast<const float*>(&bufferNorm.data[bufferViewNorm.byteOffset + accessorNorm.byteOffset]);

                            // Get indices
                            const tinygltf::Accessor& accessorIdx = model.accessors[0];
                            const tinygltf::BufferView& bufferViewIdx = model.bufferViews[accessorIdx.bufferView];
                            const tinygltf::Buffer& bufferIdx = model.buffers[bufferViewIdx.buffer];
                            const unsigned short* indices = reinterpret_cast<const unsigned short*>(&bufferIdx.data[bufferViewIdx.byteOffset + accessorIdx.byteOffset]);

                            for (size_t i = 0; i < accessorIdx.count; i += 3) {
                                int idx0 = indices[i];
                                int idx1 = indices[i + 1];
                                int idx2 = indices[i + 2];
                                // Get triangle vertices and surface normal
                                glm::vec3 v0(positions[idx0 * 3], positions[idx0 * 3 + 1], positions[idx0 * 3 + 2]);
                                glm::vec3 v1(positions[idx1 * 3], positions[idx1 * 3 + 1], positions[idx1 * 3 + 2]);
                                glm::vec3 v2(positions[idx2 * 3], positions[idx2 * 3 + 1], positions[idx2 * 3 + 2]);
                                glm::vec3 n0(normals[idx0 * 3], normals[idx0 * 3 + 1], normals[idx0 * 3 + 2]);
                                glm::vec3 n1(normals[idx1 * 3], normals[idx1 * 3 + 1], normals[idx1 * 3 + 2]);
                                glm::vec3 n2(normals[idx2 * 3], normals[idx2 * 3 + 1], normals[idx2 * 3 + 2]);
                                /*cout << "NORMAL" << endl;
                                cout << "(" << n[0] << ", " << n[1] << ", " << n[2] << ")" << endl;
                                cout << "POSITIONS" << endl;
                                cout << "(" << v0[0] << ", " << v0[1] << ", " << v0[2] << ")" << endl;
                                cout << "(" << v1[0] << ", " << v1[1] << ", " << v1[2] << ")" << endl;
                                cout << "(" << v2[0] << ", " << v2[1] << ", " << v2[2] << ")" << endl;*/
                                Geom triangle;
                                triangle.type = TRIANGLE;
                                triangle.n0 = n0;
                                triangle.n1 = n1;
                                triangle.n2 = n2;
                                triangle.v0 = v0;
                                triangle.v1 = v1;
                                triangle.v2 = v2;
                                triangles.push_back(triangle);
                            }
                        }
                    }
                }
            }
        }

        //link material
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            newGeom.materialid = atoi(tokens[1].c_str());
            if (newGeom.type != TRIANGLE) {
                cout << "Connecting Geom " << objectid << " to Material " << newGeom.materialid << "..." << endl;
            }
        }

        //load transformations
        utilityCore::safeGetline(fp_in, line);
        while (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);

            //load tranformations
            if (strcmp(tokens[0].c_str(), "TRANS") == 0) {
                newGeom.translation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            } else if (strcmp(tokens[0].c_str(), "ROTAT") == 0) {
                newGeom.rotation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            } else if (strcmp(tokens[0].c_str(), "SCALE") == 0) {
                newGeom.scale = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            }

            utilityCore::safeGetline(fp_in, line);
        }

        newGeom.transform = utilityCore::buildTransformationMatrix(
                newGeom.translation, newGeom.rotation, newGeom.scale);
        newGeom.inverseTransform = glm::inverse(newGeom.transform);
        newGeom.invTranspose = glm::inverseTranspose(newGeom.transform);

        if (newGeom.type == TRIANGLE) {
            // iterate over all triangles
            for (Geom triangle : triangles) {
                triangle.materialid = newGeom.materialid;
                triangle.translation = newGeom.translation;
                triangle.rotation = newGeom.rotation;
                triangle.scale = newGeom.scale;
                triangle.transform = newGeom.transform;
                triangle.inverseTransform = newGeom.inverseTransform;
                triangle.invTranspose = newGeom.invTranspose;
                geoms.push_back(triangle);
                num_triangles++;
            }
            num_meshes++;
        }
        else {
            geoms.push_back(newGeom);
        }
        return 1;
    }
}

int Scene::loadCamera() {
    cout << "Loading Camera ..." << endl;
    RenderState &state = this->state;
    Camera &camera = state.camera;
    float fovy;

    //load static properties
    for (int i = 0; i < 7; i++) {
        string line;
        utilityCore::safeGetline(fp_in, line);
        vector<string> tokens = utilityCore::tokenizeString(line);
        if (strcmp(tokens[0].c_str(), "RES") == 0) {
            camera.resolution.x = atoi(tokens[1].c_str());
            camera.resolution.y = atoi(tokens[2].c_str());
        } else if (strcmp(tokens[0].c_str(), "FOVY") == 0) {
            fovy = atof(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "ITERATIONS") == 0) {
            state.iterations = atoi(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "DEPTH") == 0) {
            state.traceDepth = atoi(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "FILE") == 0) {
            state.imageName = tokens[1];
        } else if (strcmp(tokens[0].c_str(), "LENS") == 0) {
            camera.lensRadius = atof(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "FOCAL") == 0) {
            camera.focalDist = atof(tokens[1].c_str());
        }
    }

    string line;
    utilityCore::safeGetline(fp_in, line);
    while (!line.empty() && fp_in.good()) {
        vector<string> tokens = utilityCore::tokenizeString(line);
        if (strcmp(tokens[0].c_str(), "EYE") == 0) {
            camera.position = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        } else if (strcmp(tokens[0].c_str(), "LOOKAT") == 0) {
            camera.lookAt = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        } else if (strcmp(tokens[0].c_str(), "UP") == 0) {
            camera.up = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        }

        utilityCore::safeGetline(fp_in, line);
    }

    //calculate fov based on resolution
    float yscaled = tan(fovy * (PI / 180));
    float xscaled = (yscaled * camera.resolution.x) / camera.resolution.y;
    float fovx = (atan(xscaled) * 180) / PI;
    camera.fov = glm::vec2(fovx, fovy);

	camera.right = glm::normalize(glm::cross(camera.view, camera.up));
	camera.pixelLength = glm::vec2(2 * xscaled / (float)camera.resolution.x
							, 2 * yscaled / (float)camera.resolution.y);

    camera.view = glm::normalize(camera.lookAt - camera.position);

    //set up render camera stuff
    int arraylen = camera.resolution.x * camera.resolution.y;
    state.image.resize(arraylen);
    std::fill(state.image.begin(), state.image.end(), glm::vec3());

    cout << "Loaded camera!" << endl;
    return 1;
}

int Scene::loadMaterial(string materialid) {
    int id = atoi(materialid.c_str());
    if (id != materials.size()) {
        cout << "ERROR: MATERIAL ID does not match expected number of materials" << endl;
        return -1;
    } else {
        cout << "Loading Material " << id << "..." << endl;
        Material newMaterial;

        //load static properties
        for (int i = 0; i < 8; i++) {
            string line;
            utilityCore::safeGetline(fp_in, line);
            vector<string> tokens = utilityCore::tokenizeString(line);
            if (strcmp(tokens[0].c_str(), "TYPE") == 0) {
                if (strcmp(tokens[1].c_str(), "DIFFUSE") == 0) {
                    newMaterial.type = (MaterialType)0;
                } else if (strcmp(tokens[1].c_str(), "MIRROR") == 0) {
                    newMaterial.type = (MaterialType)1;
                } else if (strcmp(tokens[1].c_str(), "GLOSSY") == 0) {
                    newMaterial.type = (MaterialType)2;
                } else if (strcmp(tokens[1].c_str(), "DIELECTRIC") == 0) {
                    newMaterial.type = (MaterialType)3;
                } else if (strcmp(tokens[1].c_str(), "GLASS") == 0) {
                    newMaterial.type = (MaterialType)4;
                } else if (strcmp(tokens[1].c_str(), "EMISSIVE") == 0) {
                    newMaterial.type = (MaterialType)5;
                }
            }
            if (strcmp(tokens[0].c_str(), "RGB") == 0) {
                glm::vec3 color( atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()) );
                newMaterial.color = color;
            } else if (strcmp(tokens[0].c_str(), "SPECEX") == 0) {
                newMaterial.specular.exponent = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "SPECRGB") == 0) {
                glm::vec3 specColor(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
                newMaterial.specular.color = specColor;
            } else if (strcmp(tokens[0].c_str(), "REFL") == 0) {
                newMaterial.hasReflective = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "REFR") == 0) {
                newMaterial.hasRefractive = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "REFRIOR") == 0) {
                newMaterial.indexOfRefraction = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "EMITTANCE") == 0) {
                newMaterial.emittance = atof(tokens[1].c_str());
            }
        }
        materials.push_back(newMaterial);
        return 1;
    }
}
