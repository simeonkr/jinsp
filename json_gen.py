# generate a JSON file for testing purposes

import sys
import json
import string
import random
import numpy as np

SCALE = 512

def gen_string(_):
    size = np.random.geometric(0.01)
    return ''.join([
        random.choice(string.ascii_lowercase + string.digits) 
        for _ in range(size)
    ])

def gen_number(_):
    return random.randint(1, 2**20)

def gen_key(_):
    size = np.random.geometric(0.1)
    return ''.join([
        random.choice(string.ascii_lowercase + string.digits) 
        for _ in range(size)
    ])

def gen_value(depth):
    typ = random.choices(
        ['number', 'string', 'array', 'object'],
        [1, 1, 1, 1])[0]
    return globals()[f'gen_{typ}'](depth + 1)

def gen_array(depth):
    size = int(np.random.binomial(2 * SCALE / (2 ** (depth - 1)), 0.5))
    return [
        gen_value(depth + 1) for _ in range(size)
    ]

def gen_object(depth):
    size = int(np.random.binomial(2 * SCALE / (2 ** (depth - 1)), 0.5))
    return {
        gen_key(depth + 1) : gen_value(depth + 1) for _ in range(size)
    }

def gen_json(depth=0):
    return gen_object(depth + 1)


with open(sys.argv[1], 'w') as f:
    json.dump(gen_json(), f)
