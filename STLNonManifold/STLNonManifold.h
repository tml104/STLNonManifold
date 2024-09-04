#pragma once
#include <cmath>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>

#include "./stl_reader/stl_reader.h"
#include "KDTree.h"
#include "CoreOld.h"

template<typename T1, typename T2>
auto myzip(const T1& container1, const T2& container2) {
	std::vector<std::tuple<typename T1::value_type, typename T2::value_type>> result;
	auto it1 = container1.begin();
	auto it2 = container2.begin();
	while (it1 != container1.end() && it2 != container2.end()) {
		result.emplace_back(*it1, *it2);
		++it1;
		++it2;
	}
	return result;
}

namespace STLNonManifold {

	namespace Geometry {
		using T_NUM = float;
		const T_NUM SAME_THRESHOLD = 1e-6f;

		struct Coordinate {
			T_NUM coords[3];
			int id;

			Coordinate() {
				for (int i = 0; i < 3; i++) {
					coords[i] = 0.0;
				}
				id = 0;
			}

			Coordinate(T_NUM x, T_NUM y, T_NUM z, int index = 0) {
				coords[0] = x;
				coords[1] = y;
				coords[2] = z;
				id = index;
			}

			Coordinate(const T_NUM* num_list, int index = 0) {
				for (int i = 0; i < 3; i++) {
					coords[i] = num_list[i];
				}
				id = index;
			}

			T_NUM& x() {
				return coords[0];
			}

			T_NUM& y() {
				return coords[1];
			}

			T_NUM& z() {
				return coords[2];
			}

			int GetId() const {
				return id;
			}

			void SetId(int index) {
				id = index;
			}

			T_NUM& operator[](unsigned int i) {
				return coords[i];
			}

			const T_NUM& operator[](unsigned int i) const {
				return coords[i];
			}

			bool operator==(const Coordinate& other_coord) const {
				for (int i = 0; i < 3; i++) {
					if (abs(other_coord.coords[i] - coords[i]) > SAME_THRESHOLD) {
						return false;
					}
				}

				return true;
			}
		};

		struct Vertex;
		struct Edge;
		struct Triangle;

		struct Vertex {
			std::shared_ptr<Coordinate> pointCoord;
			int id;
		};

		struct Edge {
			std::shared_ptr<Vertex> start, end;
			std::vector<std::shared_ptr<Triangle>> incident_triangles;
			int id;
		};

		struct Triangle {
			std::vector<std::shared_ptr<Edge>> edges;
			std::vector<bool> edges_senses; // true: 一致; false: 反转
			int id;
		};

		
	}

	struct STLNonManifoldChecker {

		STLNonManifoldChecker(const std::string& stl_file): mesh(stl_file) {

			// 读取所有坐标
			std::vector<STLNonManifold::Geometry::Coordinate> coordinates;
			int coords_count = 0;
			for (size_t i_solid = 0; i_solid < mesh.num_solids(); i_solid++) {
				for (size_t j_tri = mesh.solid_tris_begin(i_solid); j_tri < mesh.solid_tris_end(i_solid); j_tri++) {

					// 我靠，居然返回的是指针？？？
					STLNonManifold::Geometry::Coordinate coord0(mesh.tri_corner_coords(j_tri, 0), coords_count++);
					STLNonManifold::Geometry::Coordinate coord1(mesh.tri_corner_coords(j_tri, 1), coords_count++);
					STLNonManifold::Geometry::Coordinate coord2(mesh.tri_corner_coords(j_tri, 2), coords_count++);

					coordinates.emplace_back(coord0);
					coordinates.emplace_back(coord1);
					coordinates.emplace_back(coord2);
				}
			}
			
			// 构造KDTree并对坐标去重
			KDTree<STLNonManifold::Geometry::Coordinate, STLNonManifold::Geometry::T_NUM> kdtree(coordinates);
			std::vector<int> coordinates_indices(coords_count);
			std::vector<bool> coordinates_indices_flag(coords_count);
			for (int i = 0; i < coords_count; i++) {

				if (coordinates_indices_flag[i] == false) {
					auto same_coords = kdtree.Match(coordinates[i]);
					auto min_it = std::min_element(same_coords.begin(), same_coords.end(), [&](const STLNonManifold::Geometry::Coordinate& a, const STLNonManifold::Geometry::Coordinate& b) {return a.GetId() < b.GetId(); });
					int min_id = min_it->GetId();

					for (auto&& coord : same_coords) {
						int coord_id = coord.GetId();
						coordinates_indices[coord_id] = min_id;
						coordinates_indices_flag[coord_id] = true;
					}

				}

				// [debug]
				//std::cout << i << ": ";
				//for (auto&& coord : same_coords) {
				//	std::cout << coord.GetId() <<" ";
				//	std::cout << "(" << coord[0] << ", " << coord[1] << ", " << coord[2] << "), ";
				//}
				//std::cout << std::endl;
			}

			 //[debug]
			//for (auto&& id : coordinates_indices) {
			//	std::cout << id << ", ";
			//}
			//std::cout << std::endl;

			// 构造拓扑：构造顶点
			// 注意这里vertex的id是按照坐标合并后新赋值的
			int vertices_count = 0;
			std::map<int, std::shared_ptr<STLNonManifold::Geometry::Vertex>> vertices_map; // merged_index -> vertex
			for (int i = 0; i < coords_count;i++) {
				int vertex_merged_index = coordinates_indices[i];
				auto it = vertices_map.find(vertex_merged_index);

				// 没找到已经创建的顶点：创建
				if (it == vertices_map.end()) {

					auto vertex_ptr = std::make_shared<STLNonManifold::Geometry::Vertex>();
					auto coord_ptr = std::make_shared<STLNonManifold::Geometry::Coordinate>(coordinates[vertex_merged_index]); // 复制

					// 构造顶点
					vertex_ptr->id = vertices_count;
					vertex_ptr->pointCoord = coord_ptr;
					coord_ptr->id = vertices_count;

					vertices_map[vertex_merged_index] = vertex_ptr;
					vertices_count++;
				}
			}

			// 构造拓扑：逐三角形地去构造
			int edges_count = 0;
			int triangles_count = 0;
			std::map<std::pair<int, int>, std::shared_ptr<STLNonManifold::Geometry::Edge>> edges_map; // (merge_index, merged_index) -> edge
			for (int i = 0; i< coords_count; i += 3) {
				// 三角
				std::shared_ptr<STLNonManifold::Geometry::Triangle> triangle_ptr = std::make_shared<STLNonManifold::Geometry::Triangle>();
				triangle_ptr->id = i/3;

				// 边
				for (int j = 0; j < 3; j++) {

					int p1_index = i + j;
					int p2_index = i + (j+1)%3;
					int p1_merged_index = coordinates_indices[p1_index];
					int p2_merged_index = coordinates_indices[p2_index];

					int start_merged_index = p1_merged_index;
					int end_merged_index = p2_merged_index;

					if (p1_merged_index > p2_merged_index) {
						std::swap(p1_merged_index, p2_merged_index);
					}

					auto it = edges_map.find(std::make_pair(p1_merged_index, p2_merged_index));
					if (it == edges_map.end()) {
						// 对应（合并顶点后的）边不存在：创建
						auto edge_ptr = std::make_shared<STLNonManifold::Geometry::Edge>();

						edge_ptr->start = vertices_map[start_merged_index];
						edge_ptr->end = vertices_map[end_merged_index];
						edge_ptr->incident_triangles.emplace_back(triangle_ptr);
						edge_ptr->id = edges_count++;

						triangle_ptr->edges.emplace_back(edge_ptr);
						triangle_ptr->edges_senses.emplace_back(true);

						edges_map[std::make_pair(p1_merged_index, p2_merged_index)] = edge_ptr;
					}
					else {
						// 对应边已经存在：维护
						auto edge_ptr = it->second;
						edge_ptr->incident_triangles.emplace_back(triangle_ptr);
						triangle_ptr->edges.emplace_back(edge_ptr);
						triangle_ptr->edges_senses.emplace_back(false);
					}

				}

				triangles.emplace_back(triangle_ptr);
				triangles_count++;
			}

			verticesCount = vertices_count;
			edgesCount = edges_count;
			trianglesCount = triangles_count;

			//std::cout << "verticesCount: " << verticesCount << std::endl;
			//std::cout << "edgesCount: " << edgesCount << std::endl;
			//std::cout << "trianglesCount: " << trianglesCount << std::endl;

			LOG_INFO("verticesCount: %d", verticesCount);
			LOG_INFO("edgesCount: %d", edgesCount);
			LOG_INFO("trianglesCount: %d", trianglesCount);
		}

		void CheckNonManifold() {

			int non_manifold_count = 0;

			for (auto&& triangle_ptr : triangles) {
				//std::cout << "Triangle: " << triangle_ptr->id << std::endl;

				for (auto&& edge_ptr : triangle_ptr->edges) {
					//std::cout << "Edge: " << edge_ptr->id << std::endl;

					if (edge_ptr->incident_triangles.size() != 2) {
						non_manifold_count++;
						//std::cout << "======" << std::endl;
						//std::cout << "NonManifold: " << edge_ptr->incident_triangles.size() << std::endl;
						//std::cout << "Triangle: " << triangle_ptr->id << std::endl;
						//std::cout << "Edge: " << edge_ptr->id << std::endl;
						//std::cout << "Start Vertex: " << edge_ptr->start->id << std::endl;
						//std::cout << "(" << edge_ptr->start->pointCoord->x() <<", " << edge_ptr->start->pointCoord->y() << ", "<< edge_ptr->start->pointCoord->z() <<")" << std::endl;
						//std::cout << "End Vertex: " << edge_ptr->end->id << std::endl;
						//std::cout << "(" << edge_ptr->end->pointCoord->x() << ", " << edge_ptr->end->pointCoord->y() << ", " << edge_ptr->end->pointCoord->z() << ")" << std::endl;
						//std::cout << "======" << std::endl;

						LOG_INFO("======");
						LOG_INFO("NonManifold: %d", edge_ptr->incident_triangles.size());
						LOG_INFO("Triangle: %d", triangle_ptr->id);
						LOG_INFO("Edge: %d", edge_ptr->id);
						LOG_INFO("Start Vertex: %d (%.5lf, %.5lf, %.5lf)", edge_ptr->start->id, edge_ptr->start->pointCoord->x(), edge_ptr->start->pointCoord->y(), edge_ptr->start->pointCoord->z());
						LOG_INFO("End Vertex: %d (%.5lf, %.5lf, %.5lf)", edge_ptr->end->id, edge_ptr->end->pointCoord->x(), edge_ptr->end->pointCoord->y(), edge_ptr->end->pointCoord->z());
						LOG_INFO("======");

					}
				}
			}

			std::cout << "Total NonManifold Count: " << non_manifold_count << std::endl;
			std::cout << "CheckNonManifold end." << std::endl;

			LOG_INFO("Total NonManifold Count: %d", non_manifold_count);
			LOG_INFO("CheckNonManifold end.");

		}
		
		void Export2OBJ(const std::string& output_obj_file_path) {
			// vertex中的id其实就可以被视为离散化后的各个点，因此可以直接根据这个为依据做导出
			std::fstream f;
			f.open(output_obj_file_path, std::ios::out | std::ios::trunc);

			if (!f.is_open()) {
				throw std::runtime_error("Open output_obj_file_path failed.");
			}

			f << "# verticesCount: " << verticesCount << "\n";
			f << "# edgesCount: " << edgesCount << "\n";
			f << "# trianglesCount: " << trianglesCount << "\n";

			// v
			// 从三角形开始遍历，取出每个顶点（当然要去重）
			std::vector<std::shared_ptr<STLNonManifold::Geometry::Vertex>> vertices;
			std::unordered_set<int> vertices_id_set;
			for (auto&& triangle_ptr : triangles) {
				for (auto&& edge_ptr : triangle_ptr->edges) {
					int vertices_id = edge_ptr->start->id;
					
					if (vertices_id_set.count(vertices_id) == 0) {
						vertices.emplace_back(edge_ptr->start);
						vertices_id_set.insert(vertices_id);
					}

					vertices_id = edge_ptr->end->id;
					if (vertices_id_set.count(vertices_id) == 0) {
						vertices.emplace_back(edge_ptr->end);
						vertices_id_set.insert(vertices_id);
					}
				}
			}

			std::sort(vertices.begin(), vertices.end(), [](const std::shared_ptr<STLNonManifold::Geometry::Vertex>& a, const std::shared_ptr<STLNonManifold::Geometry::Vertex>& b) {
				return a->id < b->id;
			});

			for (auto&& v_ptr : vertices) {
				f << "v " << v_ptr->pointCoord->x() << " " << v_ptr->pointCoord->y() << " " << v_ptr->pointCoord->z() << "\n";
			}

			// f
			for (auto&& triangle_ptr : triangles) {
				std::vector<int> vertices_3_indices;
				vertices_3_indices.reserve(3);

				auto zipped = myzip(triangle_ptr->edges, triangle_ptr->edges_senses);
				for (const auto& [edge_ptr, edge_sense] : zipped) {
					if (edge_sense) {
						vertices_3_indices.emplace_back(edge_ptr->start->id);
					}
					else {
						vertices_3_indices.emplace_back(edge_ptr->end->id);
					}

				}

				f << "f";
				for (auto&& vertices_index : vertices_3_indices) {
					f <<" " << vertices_index+1;
				}
				f << "\n";
			}

			f.close();
		}

		stl_reader::StlMesh <STLNonManifold::Geometry::T_NUM, unsigned int> mesh;
		std::vector<std::shared_ptr<STLNonManifold::Geometry::Triangle>> triangles;

		int trianglesCount;
		int edgesCount;
		int verticesCount;
	};

} // namespace STLNonManifold