#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <functional>
#include <algorithm>
#include <limits>

/*
	CoordType 约束：
	- 重载了 [] 运算符
	- 重载了 == 运算符
	- 可默认构造
	- 可复制
*/

template<typename CoordType, typename T_NUM, int DIM = 3>
struct KDTree {

	// 匹配相同的容差
	const T_NUM ERROR_LIMITS = 1e-6f;

	struct KDTreeNode {
		CoordType minRangePoint;
		CoordType maxRangePoint;
		std::unique_ptr<KDTreeNode> leftNode;
		std::unique_ptr<KDTreeNode> rightNode;
		int splitDim = 0;
		bool isLeaf = false;
		CoordType leafPoint;
	};

	KDTree() {}

	KDTree(std::vector<CoordType> coords) {
		BuildTree(std::move(coords));
	}

	// 此处必须复制vector，否则将导致原vector被打乱
	void BuildTree(std::vector<CoordType> coords) {

		// 叶子节点
		auto construct_leaf_node = [&](const CoordType& coord, int now_dim) ->std::unique_ptr<KDTreeNode> {
			std::unique_ptr<KDTreeNode> node = std::make_unique<KDTreeNode>();
			
			node->isLeaf = true;
			node->splitDim = now_dim;
			node->leafPoint = coord; // 复制

			// 更新rangePoints
			for (int i = 0; i < DIM; i++) {
				node->minRangePoint[i] = coord[i];
				node->maxRangePoint[i] = coord[i];
			}

			return std::move(node);
		};

		// 非叶子节点
		auto construct_middle_node = [&](std::unique_ptr<KDTreeNode> leftNode, std::unique_ptr<KDTreeNode> rightNode, int now_dim) ->std::unique_ptr<KDTreeNode> {
			std::unique_ptr<KDTreeNode> node = std::make_unique<KDTreeNode>();

			node->isLeaf = false;
			node->splitDim = now_dim;

			// rangePoints设置默认最小值和最大值
			for (int i = 0; i < DIM; i++) {
				node->minRangePoint[i] = std::numeric_limits<T_NUM>::max();
				node->maxRangePoint[i] = std::numeric_limits<T_NUM>::min();
			}

			// 更新左右节点指针，同时更新rangePoints
			if (leftNode) {
				for (int i = 0; i < DIM; i++) {
					node->minRangePoint[i] = std::min(node->minRangePoint[i], leftNode->minRangePoint[i]);
					node->maxRangePoint[i] = std::max(node->maxRangePoint[i], leftNode->maxRangePoint[i]);
				}

				node->leftNode = std::move(leftNode);
			}

			if (rightNode) {
				for (int i = 0; i < DIM; i++) {
					node->minRangePoint[i] = std::min(node->minRangePoint[i], rightNode->minRangePoint[i]);
					node->maxRangePoint[i] = std::max(node->maxRangePoint[i], rightNode->maxRangePoint[i]);
				}

				node->rightNode = std::move(rightNode);
			}

			return std::move(node);
		};

		// 递归构造
		std::function<std::unique_ptr<KDTreeNode>(std::vector<CoordType>&, int, int, int)> recursive_construct = [&](std::vector<CoordType>& coords, int now_dim, int l, int r) -> std::unique_ptr<KDTreeNode> {
			if (l == r) {
				return std::move(construct_leaf_node(coords[l], now_dim));
			}

			if (l > r) {
				return nullptr;
			}

			int mid = (l + r) >> 1;

			// 利用nth_element划分
			std::nth_element(
				coords.begin() + l,
				coords.begin() + mid,
				coords.begin() + (r + 1),
				[&](const CoordType& a, const CoordType& b) {
					return a[now_dim] < b[now_dim];
				}
			);

			std::unique_ptr<KDTreeNode> left_node = recursive_construct(coords, (now_dim + 1) % DIM, l, mid);
			std::unique_ptr<KDTreeNode> right_node = recursive_construct(coords, (now_dim + 1) % DIM, mid+1, r);

			return std::move(construct_middle_node(std::move(left_node), std::move(right_node), now_dim));
		};

		this->root = recursive_construct(coords, 0, 0, static_cast<int>(coords.size()) - 1);
	}

	/*
		匹配相等的坐标点，并返回所有相同的点的vector
	*/
	std::vector<CoordType> Match(const CoordType& coord_to_be_matched) {
		
		std::vector<CoordType> result_coords;

		/*
			若满足剪枝条件，返回false，否则返回true
		*/
		auto is_subtree_valid = [&](KDTreeNode* now_node) -> bool {

			for (int i = 0; i < DIM; i++) {
				if (now_node->minRangePoint[i] - ERROR_LIMITS > coord_to_be_matched[i]) {
					return false;
				}

				if (now_node->maxRangePoint[i] + ERROR_LIMITS < coord_to_be_matched[i]) {
					return false;
				}
			}

			return true;
		};

		/*
			若叶子节点中的点和待匹配点在容差范围内则返回true
		*/
		auto is_leaf_valid = [&](KDTreeNode* now_leaf_node) -> bool {

			if (now_leaf_node == nullptr) {
				return false;
			}

			if (now_leaf_node->isLeaf == false) {
				return false;
			}

			for (int i = 0; i < DIM; i++) {
				if (abs(now_leaf_node->leafPoint[i] - coord_to_be_matched[i]) > ERROR_LIMITS) {
					return false;
				}
			}

			return true;
		};

		std::function<void(KDTreeNode*)> recursive_match = [&](KDTreeNode* now_root) {
			if (now_root->isLeaf) {

				if (is_leaf_valid(now_root)) {
					result_coords.emplace_back(now_root->leafPoint);
				}

				return;
			}

			// 检查当前子树是否有效
			if (is_subtree_valid(now_root) == false) {
				return;
			}

			// 检查左右子树
			if (now_root->leftNode) {
				recursive_match(now_root->leftNode.get());
			}

			if (now_root->rightNode) {
				recursive_match(now_root->rightNode.get());
			}
		};

		recursive_match(this->root.get());

		return result_coords;
	}

	std::unique_ptr<KDTreeNode> root;
};