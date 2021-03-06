#pragma once

#include "pager.h"

#pragma pack(push)
#pragma pack(4)

struct Node {
    enum Type {
        NONE,
        INTERNAL,
        LEAF
    };

    template <uint32_t BodySize>
    struct Body {
        char buff[BodySize];
    };

    bool
    IsRoot() const {
        return header.is_root;
    }

    bool
    IsDirty() const {
        return header.type != Type::NONE;
    }

    void
    SetRoot(bool is_root) {
        header.is_root = is_root;
    }

    bool
    IsLeaf() const {
        return header.type == Type::LEAF;
    }

    bool
    IsInternal() const {
        return header.type == Type::INTERNAL;
    }

    void
    SetType(Type type) {
        header.type = type;
    }
    Type
    GetType() const {
        return header.type;
    }
    void
    SetParentPage(uint32_t page_num) {
        header.parent = page_num;
    }

    uint32_t
    GetParentPage() const {
        return header.parent;
    }

    struct NodeHeader {
        uint32_t parent;
        Type type;
        bool is_root;
    };

    NodeHeader header;
};

template <int PageSize>
struct InternalNode : public Node {
    struct InternalHeader {
        uint32_t num_keys;
        uint32_t right_child;
    };

    constexpr static const uint32_t BodySize = PageSize - sizeof(Node) - sizeof(InternalHeader);
    constexpr static const uint32_t KeySize = sizeof(uint32_t);
    constexpr static const uint32_t ChildSize = sizeof(uint32_t);
    constexpr static const uint32_t CellSize = KeySize + ChildSize;
    constexpr static const uint32_t MaxCells = (BodySize / CellSize) - 1;

    /* InternalNode(InternalNode& o) : Node(o), */
    /*     internal_header(o.internal_header), body(o.body) { */
    /* } */

    InternalNode() {};

    void
    Reset() {
        SetType(Type::INTERNAL);
        internal_header.num_keys = 0;
        memset(body.buff, 0, BodySize);
    }

    void*
    CellPtr(uint32_t cell_num) {
        if (!body.buff) {
            return nullptr;
        }
        auto pos = cell_num * CellSize;
        if (pos + CellSize <= BodySize) {
            return &body.buff[pos];
        }
        return nullptr;
    }

    uint32_t
    RightChildPage() {
        return internal_header.right_child;
    }

    uint32_t*
    ChildPtr(uint32_t child_num) {
        auto& num_keys = internal_header.num_keys;
        if (child_num > num_keys) {
            return nullptr;
        }
        if (child_num == num_keys) {
            /* std::cout << __func__ << ": " << "child_num=" << child_num << " page_num=" << internal_header.right_child << std::endl; */
            return &internal_header.right_child;
        }
        auto ptr = CellPtr(child_num);
        /* std::cout << __func__ << ":" << __LINE__ << ": child_num=" << child_num << " page_num=" << *((uint32_t*)ptr) << std::endl; */
        return (uint32_t*)ptr;
    }

    Status
    GetChildPageNum(uint32_t child_num, uint32_t& page_num) {
        Status status;
        uint32_t* ptr = ChildPtr(child_num);
        if (!ptr) {
            status.type = StatusType::CHILD_OVERFLOW;
            return status;
        }
        page_num = *ptr;
        /* std::cout << __func__ << ": " << "child_num=" << child_num << " page_num=" << page_num << std::endl; */
        return status;
    }

    Status
    SetChild(uint32_t child_num, uint32_t child_page_num) {
        Status status;
        uint32_t* child = ChildPtr(child_num);
        if (!child) {
            status.type = StatusType::CHILD_OVERFLOW;
            status.err_msg = "CHILD_OVERFLOW";
            return status;
        }
        *child = child_page_num;
        return status;
    }

    uint32_t*
    KeyPtr(uint32_t key_num) {
        auto ptr = CellPtr(key_num);
        if (!ptr) {
            return (uint32_t*)ptr;
        }
        return (uint32_t*)((uint8_t*)ptr + ChildSize);
    }

    Status
    GetKey(uint32_t key_num, uint32_t& key) {
        Status status;
        auto ptr = KeyPtr(key_num);
        if (!ptr) {
            status.type = KEY_OVERFLOW;
            return status;
        }
        key = *ptr;
        return status;
    }

    Status
    SetKey(uint32_t key_num, uint32_t key) {
        Status status;
        uint32_t* ptr = KeyPtr(key_num);
        if (!ptr) {
            status.type = StatusType::KEY_OVERFLOW;
            status.err_msg = "KEY_OVERFLOW";
            return status;
        }
        *ptr = key;

        return status;
    }

    uint32_t
    FindCell(uint32_t key) {
        uint32_t min_cell = 0;
        uint32_t max_cell = NumOfKeys();
        /* Status status; */

        while (min_cell != max_cell) {
            uint32_t cell = (min_cell + max_cell) / 2;
            uint32_t it_key;
            GetKey(cell, it_key);
            if (it_key >= key) {
                max_cell = cell;
            } else {
                min_cell = cell + 1;
            }
        }
        return min_cell;
    }

    uint32_t
    NumOfKeys() const  {
        return internal_header.num_keys;
    }

    void
    SetNumOfKeys(uint32_t num) {
        internal_header.num_keys = num;
    }

    void
    SetRightChild(uint32_t rc) {
        internal_header.right_child = rc;
    }

    Status
    GetMaxKey(uint32_t& key) {
        Status status;
        if (internal_header.num_keys <= 0) {
            status.type = StatusType::EMPTY_KEY;
            status.err_msg = "EMPTY_KEY";
            return status;
        }
        auto key_num = internal_header.num_keys - 1;
        auto ptr = KeyPtr(key_num);
        key = *ptr;
        return status;
    }

    InternalHeader internal_header;
    Body<BodySize> body;
};

template <uint32_t PageSize, uint32_t KeySize, uint32_t ValueSize>
struct LeafNode : public Node {
    struct LeafHeader {
        /* uint32_t key_size; */
        /* uint32_t value_size; */
        uint32_t num_cells;
        uint32_t next_leaf;
    };

    constexpr static const uint32_t BodySize = PageSize - sizeof(Node) - sizeof(LeafHeader);
    constexpr static const uint32_t CellSize = KeySize + ValueSize;
    constexpr static const uint32_t CellsCapacity = BodySize / CellSize;
    constexpr static const uint32_t RightSplitCount = (CellsCapacity + 1) / 2;
    constexpr static const uint32_t LeftSplitCount = CellsCapacity + 1 - RightSplitCount;

    static std::string
    MetaInfoString() {
        std::stringstream ss;
        ss << "< LeafNode Meta\n";
        ss << "  PageSize: " << PageSize << "\n";
        ss << "  BodySize: " << BodySize << "\n";
        ss << "  CellSize: " << CellSize << "\n";
        ss << "  CellsCapacity: " << CellsCapacity << "\n";
        ss << "  LeftSplitCount: " << LeftSplitCount << "\n";
        ss << "  RightSplitCount: " << RightSplitCount << "\n";
        ss << ">";
        return std::move(ss.str());
    }

    LeafNode() {}

    void
    Reset() {
        SetType(Type::LEAF);
        leaf_header.num_cells = 0;
        leaf_header.next_leaf = 0;
    }

    uint32_t
    GetNextLeaf() const {
        return leaf_header.next_leaf;
    }
    void
    SetNextLeaf(uint32_t leaf) {
        leaf_header.next_leaf = leaf;
    }
    bool
    HasNextLeaf() const {
        return leaf_header.next_leaf != 0;
    }

    uint32_t
    NumOfCells() const {
        return leaf_header.num_cells;
    }
    void
    IncNumOfCells() {
        leaf_header.num_cells += 1;
    }

    void
    SetNumOfCells(uint32_t num) {
        leaf_header.num_cells = num;
    }

    void*
    CellPtr(uint32_t cell_num) {
        if (!body.buff) {
            return nullptr;
        }
        auto pos = cell_num * CellSize;
        if (pos + CellSize <= BodySize) {
            return (char*)body.buff + pos;
        }
        return nullptr;
    }

    void*
    CellKeyPtr(uint32_t cell_num) {
        return CellPtr(cell_num);
    }

    void*
    CellValPtr(uint32_t cell_num) {
        void* ret = CellPtr(cell_num);
        if (!ret) {
            return ret;
        }
        return (char*)ret + KeySize;
    }

    uint32_t
    FindCell(uint32_t key) {
        uint32_t min_cell = 0;
        uint32_t max_cell = NumOfCells();
        Status status;
        UserSchema val;

        while (min_cell != max_cell) {
            uint32_t cell = (min_cell + max_cell) / 2;
            uint32_t it_key;
            GetCellKeyVal(cell, it_key, val);
            /* std::cout << "min_cell: " << min_cell << " " << "max_cell:" << max_cell; */
            /* std::cout << " cell: " << cell << " " << "it_key:" << it_key << " key:" << key << std::endl; */
            if (key == it_key) {
                return cell;
            } else if (key < it_key) {
                max_cell = cell;
            } else {
                min_cell = cell + 1;
            }
        }

        return min_cell;
    }

    Status
    PutKey(const uint32_t& cell_num, const uint32_t& key) {
        Status status;
        auto cell_loc = CellKeyPtr(cell_num);
        if (!cell_loc) {
            status.type = StatusType::CELL_OVERFLOW;
            status.err_msg = std::string("CELL_OVERFLOW: ") + std::to_string(cell_num);
            return status;
        }
        memcpy(cell_loc, &key, sizeof(key));
        /* std::cout << "Put cell:" << cell_num << " key:" << key << std::endl; */
        return status;
    }

    Status
    PutVal(const uint32_t& cell_num, const UserSchema& val) {
        Status status;
        auto cell_loc = CellValPtr(cell_num);
        if (!cell_loc) {
            status.type = StatusType::CELL_OVERFLOW;
            status.err_msg = std::string("CELL_OVERFLOW: ") + std::to_string(cell_num);
            return status;
        }
        val.SerializeTo(cell_loc);
        return status;
    }

    Status
    GetCellKeyVal(const uint32_t& cell_num, uint32_t& key, UserSchema& val) {
        Status status;
        auto cell_loc = CellPtr(cell_num);
        if (!cell_loc) {
            status.type = StatusType::CELL_OVERFLOW;
            status.err_msg = std::string("CELL_OVERFLOW: ") + std::to_string(cell_num);
            return status;
        }
        memcpy(&key, cell_loc, sizeof(key));
        val.DeserializeFrom((char*)cell_loc + sizeof(key));
        return status;
    }

    Status
    GetMaxKey(uint32_t& key) {
        Status status;
        if (leaf_header.num_cells == 0) {
            status.type = EMPTY_KEY;
            status.err_msg = "EMPTY_KEY";
            return status;
        }

        uint32_t* ptr = (uint32_t*)CellKeyPtr(leaf_header.num_cells - 1);
        key = *ptr;
        return status;
    }

    LeafHeader leaf_header;
    Body<BodySize> body;
};

using InternalPage = InternalNode<Pager::PAGE_SIZE>;
using LeafPage = LeafNode<Pager::PAGE_SIZE, sizeof(UserSchema::id), sizeof(UserSchema)>;

struct PageHelper {
    static InternalPage*
    AsInternal(void* page) {
        if (!page) {
            return nullptr;
        }
        if (((Node*)page)->IsInternal()) {
            return (InternalPage*)page;
        }
        return nullptr;
    }

    static LeafPage*
    AsLeaf(void* page) {
        if (!page) {
            return nullptr;
        }
        if (((Node*)page)->IsLeaf()) {
            return (LeafPage*)page;
        }
        return nullptr;
    }

    static Status
    GetMaxKey(void* page, uint32_t& key) {
        Status status;
        InternalPage* ipage = AsInternal(page);
        if (!ipage) {
            LeafPage* lpage = AsLeaf(page);
            if (!lpage) {
                status.type = StatusType::PAGE_INVALID;
                return status;
            }
            return lpage->GetMaxKey(key);
        }

        return ipage->GetMaxKey(key);
    }
};

#pragma pack(pop)
