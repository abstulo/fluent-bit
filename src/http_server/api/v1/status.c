/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Fluent Bit
 *  ==========
 *  Copyright (C) 2019      The Fluent Bit Authors
 *  Copyright (C) 2015-2018 Treasure Data Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <fluent-bit/flb_info.h>
#include <fluent-bit/flb_pack.h>
#include <fluent-bit/flb_http_server.h>
#include <fluent-bit/flb_mem.h>
#include <fluent-bit/flb_storage.h>

#define FLB_UPTIME_ONEDAY  86400
#define FLB_UPTIME_ONEHOUR  3600
#define FLB_UPTIME_ONEMINUTE  60

/* API: List all built-in plugins */
static void cb_status(mk_request_t *request, void *data)
{
    flb_sds_t out_buf;
    size_t out_size;

    msgpack_packer mp_pck;
    msgpack_sbuffer mp_sbuf;

    struct flb_hs *hs = data;
    struct flb_config *config = hs->config;
    struct cio_stats storage_st;

    cio_stats_get(config->cio, &storage_st);

    /* initialize buffers */
    msgpack_sbuffer_init(&mp_sbuf);
    msgpack_packer_init(&mp_pck, &mp_sbuf, msgpack_sbuffer_write);

    msgpack_pack_map(&mp_pck, 1);
    msgpack_pack_str(&mp_pck, 13);
    msgpack_pack_str_body(&mp_pck, "storage_layer", 13);

    msgpack_pack_map(&mp_pck, 3);

    msgpack_pack_str(&mp_pck, 12);
    msgpack_pack_str_body(&mp_pck, "total_chunks", 12);
    msgpack_pack_int64(&mp_pck, storage_st.chunks_total);

    msgpack_pack_str(&mp_pck, 10);
    msgpack_pack_str_body(&mp_pck, "mem_chunks", 10);
    msgpack_pack_int(&mp_pck, storage_st.chunks_mem);

    msgpack_pack_str(&mp_pck, 9);
    msgpack_pack_str_body(&mp_pck, "fs_chunks", 9);
    msgpack_pack_map(&mp_pck, 2);

    // TODO look at displaying total fs chunks

    msgpack_pack_str(&mp_pck, 2);
    msgpack_pack_str_body(&mp_pck, "up", 2);
    msgpack_pack_int(&mp_pck, storage_st.chunks_fs_up);
    msgpack_pack_str(&mp_pck, 4);
    msgpack_pack_str_body(&mp_pck, "down", 4);
    msgpack_pack_int(&mp_pck, storage_st.chunks_fs_down);

    fprintf(stdout, "\n===== Storage Layer =====\n");
    fprintf(stdout, "total chunks     : %i\n", storage_st.chunks_total);
    fprintf(stdout, "├─ mem chunks    : %i\n", storage_st.chunks_mem);
    fprintf(stdout, "└─ fs chunks     : %i\n", storage_st.chunks_fs);
    fprintf(stdout, "   ├─ up         : %i\n", storage_st.chunks_fs_up);
    fprintf(stdout, "   └─ down       : %i\n", storage_st.chunks_fs_down);

    /* Export to JSON */
    out_buf = flb_msgpack_raw_to_json_sds(mp_sbuf.data, mp_sbuf.size);
    msgpack_sbuffer_destroy(&mp_sbuf);
    if (!out_buf) {
        return;
    }
    out_size = flb_sds_len(out_buf);

    mk_http_status(request, 200);
    mk_http_send(request, out_buf, out_size, NULL);
    mk_http_done(request);

    flb_sds_destroy(out_buf);
}

/* Perform registration */
int api_v1_status(struct flb_hs *hs)
{
    mk_vhost_handler(hs->ctx, hs->vid, "/api/v1/status", cb_status, hs);
    return 0;
}
