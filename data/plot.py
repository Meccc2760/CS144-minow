import re
from datetime import datetime

import matplotlib.pyplot as plt


# =========================
# Parameters
# =========================

LOAD_FILE = "data_MTN.txt"
SAVE_FILE = "rtt_mtn.png"

TITLE = "MTN RTT over Time"


# =========================
# Parse ping data
# =========================

timestamps = []
rtts = []

# Example:
# [1790157474.410371] 64 bytes from 110.75.170.150:
# icmp_seq=11231 ttl=46 time=57.1 ms

pattern = re.compile(
    r"\[(\d+\.\d+)\].*icmp_seq=(\d+).*time=([\d.]+)\s*ms"
)

with open(LOAD_FILE, "r") as file:
    for line in file:
        match = pattern.search(line)

        if match:
            timestamp = float(match.group(1))
            rtt = float(match.group(3))

            timestamps.append(datetime.fromtimestamp(timestamp))
            rtts.append(rtt)


# =========================
# Basic information
# =========================

print(f"Loaded {len(rtts)} successful ping replies")

if rtts:
    print(f"Minimum RTT: {min(rtts):.3f} ms")
    print(f"Maximum RTT: {max(rtts):.3f} ms")
    print(f"Average RTT: {sum(rtts) / len(rtts):.3f} ms")


# =========================
# Plot
# =========================

plt.figure(figsize=(14, 6))

plt.plot(
    timestamps,
    rtts,
    linewidth=0.8,
)

plt.xlabel("Time")
plt.ylabel("RTT (ms)")
plt.title(TITLE)

plt.grid(alpha=0.3)
plt.tight_layout()

plt.savefig(SAVE_FILE, dpi=200)
plt.show()