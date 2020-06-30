{"id":0, "argv":["LOADER", LOADER_ARGS, "-W", "1", "-P", "9101", "-I", "NACLPORTS_BIN/email_pipe_start.nexe", "1"], "in": [], "out": ["1:a1.0b16", "2:a1.0b16"]}
{"id":1, "argv":["LOADER", LOADER_ARGS, "-W", "1", "-P", "9103",       "NACLPORTS_BIN/dspam-app.nexe", "-h", "DATA_DIR/email", "-u", "testuser0", "-m", "-n", "-l"], "in": [0], "out": ["3:a0b64"]}
{"id":2, "argv":["LOADER", LOADER_ARGS, "-W", "1", "-P", "9102",       "NACLPORTS_BIN/clamscan.nexe", "--disable-cache", "--database=DATA_DIR/email/virus_db", "--for-nacl-pipeline", "-"], "in": [0], "out": ["3:a0b64"]}
{"id":3, "argv":["LOADER", LOADER_ARGS, "-W", "2", "-P", "9104", "-o", "NACLPORTS_BIN/email_pipe_end.nexe", "0"], "in": [1,2], "out": []}
