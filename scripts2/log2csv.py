import os

def log2csv():
    src_file_path = r'D:\data\02_CableTemperatureDetector\log/heta-cable.2024-09-18.0.log15495187616985280.tmp'
    # src_file_path = r'D:\data\02_CableTemperatureDetector\log/heta-cable.2024-09-17.0.log15408741163688121.tmp'
    # src_file_path = r'D:\data\02_CableTemperatureDetector\log/heta-cable.2024-09-16.0.log15322323894386850.tmp'
    with open(src_file_path, 'r') as f:
        data_lines = f.readlines()

    out_data_lines = []
    for i, one_line in enumerate(data_lines):
        if 'ad2' in one_line:
            continue
        a, b = one_line.split('[')
        b = b.replace(' ', '').replace(']', '')
        aa = a.split(' ')
        if aa[2] != '00000002303936393739393630353737':
            continue
        s = aa[0] + ',=,+,-,'
        if '63' in aa[3]:
            s += ' [ 0 ] ,0,'
        elif '127' in aa[3]:
            s += ' [ 1 ] ,64,'
        elif '191' in aa[3]:
            s += ' [ 2 ] ,128,'
        elif '255' in aa[3]:
            s += ' [ 3 ] ,192,'
        else:
            continue
        s += b
        # bb = b.split(',')
        out_data_lines.append(s)
        pass

    save_path = src_file_path.replace('.tmp', '.csv')
    with open(save_path, 'w') as f:
        f.writelines(out_data_lines)
    print('over')

if __name__ == '__main__':
    log2csv()
