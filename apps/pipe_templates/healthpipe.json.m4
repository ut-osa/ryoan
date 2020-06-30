{"id":0, "argv":["LOADER", LOADER_ARGS, "-W", "1", "-P", "9001", "-I", "NACLPORTS_BIN/svmsgd_model_loader_pipeline.nexe", "DATA_DIR/health/sample_models.models"], "in": [], "out": ["1:a1.0b3500"]}
{"id":1, "argv":["LOADER", LOADER_ARGS, "-W", "1", "-P", "9002",       "NACLPORTS_BIN/svmsgd_classify_pipeline.nexe"], "in": [0], "out": ["2:a0b64"]}
{"id":2, "argv":["LOADER", LOADER_ARGS, "-W", "1", "-P", "9003", "-o", "NACLPORTS_BIN/svmsgd_end_pipeline.nexe"], "in": [1], "out": []}
