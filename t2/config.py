def get_config(num):
    with open("config.txt", "r") as f:
        lines = f.readlines()

    line = lines.index(f"MAQUINA {num}\n")
    config_lines = lines[line+1:line+4]
    config = {}
    for config_line in config_lines:
        key, value = config_line.strip().split()
        config[key] = value

    return config