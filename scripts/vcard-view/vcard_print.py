#!/usr/bin/env python3

# Some parts of code are written with the help of free Google AI.

import os
import sys
import vobject
#import re   # I know about PEP 8, but see below.

path = sys.argv[1]

def fix_vcard_21(raw_text):
    import re
    fixed_lines = []
    for line in raw_text.splitlines():
        if line.startswith('ADR;'):
            for t in 'PREF', 'DOM', 'INTL', 'POSTAL', 'PARCEL', 'HOME', 'WORK':
                line = re.sub(rf';{t}(?=;|:)', f';TYPE={t}', line)
        if line.startswith('TEL;'):
            for t in 'PREF', 'WORK', 'HOME', 'VOICE', 'FAX', 'MSG', 'CELL', 'PAGER', 'BBS', 'MODEM', 'CAR', 'ISDN', 'VIDEO':
                line = re.sub(rf';{t}(?=;|:)', f';TYPE={t}', line)
        if line.startswith('EMAIL;'):
            for t in 'PREF', 'WORK', 'HOME', 'INTERNET':
                line = re.sub(rf';{t}(?=;|:)', f';TYPE={t}', line)
        fixed_lines.append(line)
    return '\n'.join(fixed_lines)

count = 1

with open(path, 'r', encoding='utf-8') as f:
    raw_content = f.read()

n = raw_content.find("VERSION:2.1", 0, 39)
if n == -1:
    contacts = vobject.readComponents(raw_content, allowQP = True)
else:
    contacts = vobject.readComponents(fix_vcard_21(raw_content), allowQP = True)

for contact in contacts:
    print(f"Contact: {count}")
    if 'fn' in contact.contents:
        print('Name: ' + contact.fn.value)
    else:
        print('Name: ' + contact.n.value)

    if 'nickname' in contact.contents:
        print('Nickname: ' + contact.nickname.value)

    if 'tel' in contact.contents:
        for tel in contact.contents['tel']:
            vtype = tel.params.get('TYPE', [])
            if len(vtype) == 0:
                print('TEL: ' + tel.value)
            else:
                print('TEL: ' + ', '.join(vtype) + ': ' + tel.value)

    if 'email' in contact.contents:
        for email in contact.contents['email']:
            vtype = email.params.get('TYPE', [])
            if len(vtype) == 0:
                print('EMAIL: ' + email.value)
            else:
                print('EMAIL: ' + ', '.join(vtype) + ': ' + email.value)

    if 'adr' in contact.contents:
        for address in contact.adr_list:
            components = []
            for attr in ['box', 'extended', 'street', 'city', 'region', 'code', 'country']:
                val = getattr(address.value, attr, '')
                if isinstance(val, list):
                    val = " ".join([str(i).strip() for i in val if i])
                components.append(str(val).strip())

            full_address = ", ".join([c for c in components if c])
            addr_params = getattr(address, 'params', {})
            addr_type_list = addr_params.get('TYPE', ['Not specified'])
            if isinstance(addr_type_list, list):
                addr_type = "-".join([str(t).upper() for t in addr_type_list])
            else:
                addr_type = str(addr_type_list).upper()
            if len(addr_type) == 0:
                print('ADR: ' + full_address)
            else:
                print('ADR: ' + addr_type + ': ' + full_address)

    if 'url' in contact.contents:
        print('URL: ' + contact.url.value)
    if 'org' in contact.contents:
        print('ORG: ' + ', '.join(contact.org.value))
    if 'title' in contact.contents:
        print('TITLE: ' + contact.title.value)
    if 'role' in contact.contents:
        print('ROLE: ' + contact.role.value)
    if 'bday' in contact.contents:
        print('BDAY: ' + contact.bday.value)
    if 'photo' in contact.contents:
        print('PHOTO: yes')
    else:
        print('PHOTO: no')
    if 'categories' in contact.contents:
        print('CATEGORIES: ' + contact.categories.value)
    if 'note' in contact.contents:
        print('NOTE: ' + contact.note.value)

    print('______________________________')
    count += 1

sys.exit(0)
