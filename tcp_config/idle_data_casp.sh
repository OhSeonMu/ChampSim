{
    "executable_name": "champsim",
    "block_size": 64,
    "page_size": 4096,
    "heartbeat_frequency": 10000000,
    "num_cores": 1,

    "ooo_cpu": [
        {
            "frequency": 4000,
            "ifetch_buffer_size":64,
            "decode_buffer_size":32,
            "dispatch_buffer_size":32,
            "rob_size": 352,
            "lq_size": 128,
            "sq_size": 72,
            "fetch_width": 6,
            "decode_width": 6,
            "dispatch_width": 6,
            "execute_width": 4,
            "lq_width": 2,
            "sq_width": 2,
            "retire_width": 5,
            "mispredict_penalty": 1,
            "scheduler_size": 128,
            "decode_latency": 1,
            "dispatch_latency": 1,
            "schedule_latency": 0,
            "execute_latency": 0,
            "branch_predictor": "bimodal",
            "btb": "basic_btb"
        }
    ],

    "DIB": {
        "window_size": 16,
        "sets": 32,
        "ways": 8
    },

    "L1I": {
        "sets": 64,
        "ways": 8,
        "rq_size": 64,
        "wq_size": 64,
        "pq_size": 32,
        "mshr_size": 8,
        "latency": 4,
        "max_tag_check": 2,
        "max_fill": 2,
        "prefetch_as_load": false,
        "virtual_prefetch": true,
        "prefetch_activate": "LOAD,PREFETCH",
        "prefetcher": "no_instr"
    },

    "L1D": {
        "sets": 64,
        "ways": 12,
        "rq_size": 64,
        "wq_size": 64,
        "pq_size": 8,
        "mshr_size": 16,
        "latency": 5,
        "max_tag_check": 2,
        "max_fill": 2,
        "prefetch_as_load": false,
        "virtual_prefetch": false,
        "prefetch_activate": "LOAD,PREFETCH",
        "prefetcher": "no"
    },

    "L2C": {
        "sets": 512,
        "ways": 8,
        "rq_size": 32,
        "wq_size": 32,
        "pq_size": 16,
        "mshr_size": 32,
        "latency": 10,
        "max_tag_check": 1,
        "max_fill": 1,
        "prefetch_as_load": false,
        "virtual_prefetch": false,
        "prefetch_activate": "LOAD,PREFETCH",
        "prefetcher": "no"
    },

    "ITLB": {
        "sets": 16,
        "ways": 4,
        "rq_size": 16,
        "wq_size": 16,
        "pq_size": 0,
        "mshr_size": 8,
        "latency": 1,
        "max_tag_check": 2,
        "max_fill": 2,
        "prefetch_as_load": false
    },

    "DTLB": {
        "sets": 16,
        "ways": 4,
        "rq_size": 16,
        "wq_size": 16,
        "pq_size": 0,
        "mshr_size": 8,
        "latency": 1,
        "max_tag_check": 2,
        "max_fill": 2,
        "prefetch_as_load": false
    },

    "STLB": {
        "sets": 128,
        "ways": 12,
        "rq_size": 32,
        "wq_size": 32,
        "pq_size": 0,
        "mshr_size": 16,
        "latency": 8,
        "max_tag_check": 1,
        "max_fill": 1,
        "prefetch_as_load": false
    },

    "PB": {
        "sets": 1,
        "ways": 32,
        "rq_size": 32,
        "wq_size": 32,
        "pq_size": 32,
        "mshr_size": 16,
        "latency": 2,
        "max_tag_check": 1,
        "max_fill": 1,
        "prefetch_as_load": true,
	"is_pb": true
    },

    "PTW": {
    	"pscl5_set": 1,
	"pscl5_way": 2,
	"pscl4_set": 1,
	"pscl4_way": 2,
	"pscl3_set": 1,
	"pscl3_way": 4,
	"pscl2_set": 4,
	"pscl2_way": 8,
	"rq_size": 16,
	"mshr_size": 5,
	"max_read": 2,
	"max_write": 2
    },

    "LLC": {
        "frequency": 4000,
        "sets": 2048,
        "ways": 16,
        "rq_size": 32,
        "wq_size": 32,
        "pq_size": 32,
        "mshr_size": 64,
        "latency": 20,
        "max_tag_check": 1,
        "max_fill": 1,
        "prefetch_as_load": false,
        "virtual_prefetch": false,
        "prefetch_activate": "LOAD,PREFETCH",
        "prefetcher": "casp",
        "replacement": "lru"
    },

    "physical_memory": {
        "frequency": 3200,
        "channels": 1,
        "ranks": 1,
        "banks": 8,
        "rows": 65536,
        "columns": 128,
        "channel_width": 8,
        "wq_size": 64,
        "rq_size": 64,
        "tRP": 11,
        "tRCD": 11,
        "tCAS": 11,
        "turn_around_time": 7.5,
"idle_memory": 1
    },

    "virtual_memory": {
        "pte_page_size": 4096,
        "num_levels": 4,
        "minor_fault_penalty": 200
    }
}
