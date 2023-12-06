import pexpect
import os

def arp_test():
    result = pexpect.run('arping -c 1 -I br0 192.160.144.2').decode('utf-8')
    if '1 packets transmitted, 1 packets received' or 'Received 1 response' in result:
        print('ARP test. Result: OK')
    else:
        print('ARP test. Result: FAIL')


def ping_test():
    result = pexpect.run('ping -c 3 192.160.144.2').decode('utf-8')
    if '3 packets transmitted, 3 received' in result:
        print('ICMP(ping) test. Result: OK')
    else:
        print('ICMP(ping) test. Result: FAIL')


def udp_test():
    test_packet = 'HELLO'
    result = pexpect.run('./udp_test').decode('utf-8')
    if result == test_packet:
        print('UDP test. Result: OK')
    else:
        print('UDP test. Result: FAIL')

def tcp_test():
    result = pexpect.run('./test-tcp.sh').decode('utf-8')
    if 'HTTP/1.1 200 OK' in result:
        print('TCP(http) test. Result: OK')
    else:
        print(result)
        print('TCP(http) test. Result: FAIL')

if __name__ == '__main__':
    arp_test()
    ping_test()
    udp_test()
    tcp_test()
