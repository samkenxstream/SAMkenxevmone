// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2022 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "../state/mpt_hash.hpp"
#include "../state/rlp.hpp"
#include "statetest.hpp"
#include <evmone/eof.hpp>
#include <gtest/gtest.h>

namespace evmone::test
{
void run_state_test(const StateTransitionTest& test, evmc::VM& vm)
{
    for (const auto& [rev, cases] : test.cases)
    {
        for (size_t case_index = 0; case_index != cases.size(); ++case_index)
        {
            SCOPED_TRACE(std::string{evmc::to_string(rev)} + '/' + std::to_string(case_index));
            // if (rev != EVMC_FRONTIER)
            //     continue;
            // if (case_index != 3)
            //     continue;

            const auto& expected = cases[case_index];
            const auto tx = test.multi_tx.get(expected.indexes);
            auto state = test.pre_state;

            // Validate deployed EOF containers before running state test.
            // Any invalid EOF in state make the test incorrect.
            for (const auto& [addr, acc] : state.get_accounts())
            {
                if (is_eof_container(acc.code))
                {
                    if (const auto result = validate_eof(rev, acc.code);
                        result != EOFValidationError::success)
                    {
                        throw std::invalid_argument(
                            "EOF container at " + hex0x(addr) +
                            " is invalid: " + std::string(get_error_message(result)));
                    }
                }
            }

            const auto res = state::transition(state, test.block, tx, rev, vm);
            if (holds_alternative<state::TransactionReceipt>(res))
                EXPECT_EQ(logs_hash(get<state::TransactionReceipt>(res).logs), expected.logs_hash);
            else
                EXPECT_TRUE(expected.exception);

            EXPECT_EQ(state::mpt_hash(state.get_accounts()), expected.state_hash);
        }
    }
}
}  // namespace evmone::test
