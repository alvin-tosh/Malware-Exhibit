import socket

def scan_for_threats(ip_address, port_range):
  for port in port_range:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    result = sock.connect_ex((ip_address, port))
    if result == 0:
      print(f"Threat detected on port {port}")
    else:
      print(f"No threat detected on port {port}")
    sock.close()

# Example usage
ip_address = "192.168.1.1"
port_range = range(1, 100) # scan ports 1-100
scan_for_threats(ip_address, port_range)