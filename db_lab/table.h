#pragma once

#include <memory>
#include <string>
#include <sstream>
#include <assert.h>

#include "status.h"
#include "pager.h"
#include "node.h"
#include "user_schema.h"

struct Table : public std::enable_shared_from_this<Table> {
    /* uint32_t num_rows = 0; // TODO: Remove */
    uint32_t root_page_num;
    std::shared_ptr<Pager> pager = nullptr;
    static std::shared_ptr<Table>
    Open(const std::string& db_path) {
        auto pager = Pager::Open(db_path);
        auto table = std::make_shared<Table>();
        table->pager = pager;
        /* table->num_rows = pager->PageNums() * Pager::ROWS_PER_PAGE; // TODO: Remove */
        table->root_page_num = 0;
        if (pager->num_pages == 0) {
            void* page;
            auto status = pager->GetPage(0, page);
            assert(status.ok());
            LeafPage* leaf = new (page) LeafPage();
            leaf->Reset();
            leaf->SetKeySize(sizeof(uint32_t));
            leaf->SetValSize(sizeof(UserSchema));
            leaf->SetRoot(true);
        }
        return table;
    }

    uint32_t
    NumOfPages() const {
        return pager->num_pages;
    }

    std::string
    ToString() {
        std::stringstream ss;
        ss << "<Table: RootPageNum=" << root_page_num << " PageNums=" << pager->num_pages << ">";
        return ss.str();
    }

    void
    Close() {
        pager->Close();
    }

    struct Cursor {
        void*
        Value() {
            LeafPage* leaf;
            auto status = GetLeafPage(leaf);
            if (!status.ok()) {
                return nullptr;
            }
            return leaf->CellValPtr(cell_num);
        }

        Status
        GetLeafPage(LeafPage*& leaf) {
            void* page;
            auto status = table->pager->GetPage(page_num, page);
            if (!status.ok()) {
                return status;
            }
            leaf = new (page) LeafPage();
            return status;
        }

        Status
        Insert(uint32_t key, UserSchema& row) {
            Status status;
            LeafPage* leaf = nullptr;
            status = GetLeafPage(leaf);
            if (!status.ok()) {
                return status;
            }
            if (leaf->NumOfCells() >= leaf->CellsCapacity()) {
                // TODO: InsertAndSplit
                return status;
            }

            if (cell_num < leaf->NumOfCells()) {
                for (uint32_t i = leaf->NumOfCells(); i > cell_num; --i) {
                    memcpy(leaf->CellPtr(i), leaf->CellPtr(i-1), leaf->CellSize());
                }
            }

            leaf->IncNumOfCells();
            status = leaf->PutKey(cell_num, key);
            if (!status.ok()) {
                return status;
            }
            status = leaf->PutVal(cell_num, row);

            return status;
        }

        Status
        Advance() {
            Status status;
            if (end_of_table) {
                status.type = StatusType::CURSOR_END_OF_FILE;
                status.err_msg = "CURSOR_END_OF_FILE";
                return status;
            }
            LeafPage* leaf;
            status = GetLeafPage(leaf);
            if (!status.ok()) {
                return status;
            }

            cell_num += 1;

            if (cell_num >= leaf->NumOfCells()) {
                auto next_page_num = leaf->GetNextLeaf();
                if (next_page_num == 0) {
                    end_of_table = true;
                } else {
                    page_num = next_page_num;
                    cell_num = 0;
                }
            }

            return status;
        }

        std::shared_ptr<Table> table;
        uint32_t page_num;
        uint32_t cell_num;
        bool end_of_table;
    };

    /* void */
    /* Append(UserSchema& row) { */
    /*     auto c = LastCursor(); */
    /*     row.SerializeTo(c->Value()); */
    /*     num_rows += 1; // TODO: Remove */
    /* } */

    std::shared_ptr<Cursor>
    StartCursor() {
        auto c = std::make_shared<Cursor>();
        c->table = shared_from_this();
        c->page_num = 0;

        LeafPage* leaf;
        auto status = c->GetLeafPage(leaf);
        if (!status.ok()) {
            return nullptr;
        }

        c->end_of_table = (leaf->NumOfCells() == 0);
        return c;
    }
    /* std::shared_ptr<Cursor> */
    /* LastCursor() { */
    /*     auto c = std::make_shared<Cursor>(); */
    /*     c->table = shared_from_this(); */
    /*     c->row_num = num_rows; // TODO: Remove */
    /*     c->end_of_table = true; */
    /*     return c; */
    /* } */
};
