/*******************************************************************************
* Copyright 2019-2020 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <sstream>

#include "dnnl_common.hpp"
#include "dnnl_memory.hpp"
#include "parser.hpp"

#include "matmul/matmul.hpp"

namespace matmul {

void check_correctness(const settings_t &s) {
    std::vector<std::pair<dnnl_data_type_t, int>> bia_cfg;
    for (const auto &i_bia_dt : s.bia_dt) {
        if (i_bia_dt == dnnl_data_type_undef) {
            bia_cfg.emplace_back(i_bia_dt, 0);
            continue;
        }
        for (const auto &i_bia_mask : s.bia_mask)
            bia_cfg.emplace_back(i_bia_dt, i_bia_mask);
    }

    for_(const auto &i_cfg : s.cfg)
    for_(const auto &i_stag : s.stag)
    for_(const auto &i_wtag : s.wtag)
    for_(const auto &i_dtag : s.dtag)
    for_(const auto &i_ld_src : s.ld_src)
    for_(const auto &i_ld_wei : s.ld_wei)
    for_(const auto &i_ld_dst : s.ld_dst)
    for_(auto i_runtime_mb : s.runtime_mb)
    for_(auto i_runtime_m : s.runtime_m)
    for_(auto i_runtime_n : s.runtime_n)
    for_(auto i_runtime_k : s.runtime_k)
    for (const auto &i_bia_cfg : bia_cfg) {
        const prb_t p(s.desc, i_cfg, i_stag, i_wtag, i_dtag, i_ld_src, i_ld_wei,
                i_ld_dst, i_runtime_mb, i_runtime_m, i_runtime_n, i_runtime_k,
                i_bia_cfg.first, i_bia_cfg.second, s.attr);
        std::stringstream ss;
        ss << p;
        const std::string cpp_pstr = ss.str();
        const char *pstr = cpp_pstr.c_str();
        BENCHDNN_PRINT(1, "run: %s\n", pstr);

        res_t res {};
        const int status = doit(&p, &res);

        bool want_perf_report = false;
        parse_result(res, want_perf_report, s.allow_unimpl, status, pstr);

        if (want_perf_report && bench_mode & PERF) {
            perf_report_t pr(s.perf_template);
            pr.report(&p, &res, pstr);
        }

        benchdnn_stat.tests++;
    }
}

int bench(int argc, char **argv) {
    driver_name = "matmul";
    using namespace parser;
    static settings_t s;
    for (; argc > 0; --argc, ++argv) {
        const bool parsed_options = parse_bench_settings(argv[0])
                || parse_batch(bench, argv[0])
                || parse_cfg(s.cfg, str2cfg, argv[0])
                || parse_tag(s.stag, argv[0], "stag")
                || parse_tag(s.wtag, argv[0], "wtag")
                || parse_tag(s.dtag, argv[0], "dtag")
                || parse_vector_option(s.ld_src, atoi, argv[0], "ld_src")
                || parse_vector_option(s.ld_wei, atoi, argv[0], "ld_wei")
                || parse_vector_option(s.ld_dst, atoi, argv[0], "ld_dst")
                || parse_vector_option(
                        s.runtime_mb, str2bool, argv[0], "runtime_mb")
                || parse_vector_option(
                        s.runtime_m, str2bool, argv[0], "runtime_m")
                || parse_vector_option(
                        s.runtime_n, str2bool, argv[0], "runtime_n")
                || parse_vector_option(
                        s.runtime_k, str2bool, argv[0], "runtime_k")
                || parse_dt(s.bia_dt, argv[0], "bia_dt")
                || parse_vector_option(s.bia_mask, atoi, argv[0], "bia_mask")
                || parse_attr(s.attr, argv[0])
                || parse_allow_unimpl(s.allow_unimpl, argv[0])
                || parse_perf_template(s.perf_template, s.perf_template_def,
                        s.perf_template_csv, argv[0])
                || parse_reset(s, argv[0]);
        if (!parsed_options) {
            catch_unknown_options(argv[0]);

            SAFE_V(str2desc(&s.desc, argv[0]));
            check_correctness(s);
        }
    }

    return parse_last_argument();
}

} // namespace matmul
