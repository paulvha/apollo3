# Created by Huseyin Meric Yigit
# part taken from https://github.com/hmyit/crc32-python/blob/master/crc32.py
class crc():

    def __init__(self):
        self.table = []
        for byte in range(256):
            crc = 0
            for bit in range(8):
                if (byte ^ crc) & 1:
                    crc = (crc >> 1) ^ 0xEDB88320
                else:
                    crc >>= 1
                byte >>= 1
            self.table.append(crc)

    def crc32(self, data, len):
        value = 0xffffffff
        i = 0
        while i < len:
            ch = data[i]
            value = self.table[(ch ^ value) & 0x000000ff] ^ (value >> 8)
            i += 1

        return (0xffffffff - value)
