#!/usr/bin/env python
import os
import sys
import argparse
import re

parser = argparse.ArgumentParser()
parser.add_argument('-f', dest='fromfmt', help='from format', default='auto')
parser.add_argument('-t', dest='tofmt', help='to format', required=True)
parser.add_argument('-n', dest='index', help='index', type=int, default=0)
parser.add_argument('--outdir', help='output directory')
parser.add_argument('--renumber', action='store_true', help='output directory')
parser.add_argument('--filter-color', dest='filter_color', help='filter multicolor', choices=['yes','no','all'], default='all')
parser.add_argument('--filter-size', dest='filter_size', help='filter size', type=int)
parser.add_argument('--filter-square', dest='filter_square', help='filter square', choices=['yes','no','all'], default='all')
parser.add_argument('--filter-unique', dest='filter_unique', help='filter unique', choices=['yes','no','all'], default='all')
parser.add_argument('--limit', help='limit number', type=int)
parser.add_argument('filenames', help='input file', nargs='+')
args = parser.parse_args()


class NonoData:
    def __init__(self):
        self.width = 0
        self.height = 0
        self.multicolor = False
        self.unique = None # unknown
        self.rows = []
        self.cols = []
        self.goal = []

    def validate(self):
        if self.width <= 0 or self.height <= 0:
            raise ValueError
        if self.width != len(self.cols):
            raise ValueError, '%d, %d' % (self.width, len(self.cols))
        if self.height != len(self.rows):
            raise ValueError

        for row in self.rows:
            if not row or row == [0]:
                continue
            for x in row:
                if not 1 <= x <= self.width:
                    raise ValueError
            if sum(row) + len(row) - 1 > self.width:
                raise ValueError

        for col in self.cols:
            if not col or col == [0]:
                continue
            for x in col:
                if not 1 <= x <= self.height:
                    raise ValueError
            if sum(col) + len(col) - 1 > self.height:
                raise ValueError

        if self.goal:
            for i, row in enumerate(self.rows):
                if map(len, re.findall('X+', self.goal[i])) != row:
                    raise ValueError, 'error row %d' % i
            for i, col in enumerate(self.cols):
                if map(len, re.findall('X+', ''.join(x[i] for x in self.goal))) != col:
                    raise ValueError, 'error column %d' % i

        if not sum(sum(x) for x in self.rows) == sum(sum(x) for x in self.cols):
            raise ValueError

        return True



class FormatParser:
    name = 'unknown'
    suffix = ''
    @classmethod
    def check_filename(cls, filename):
        if cls.suffix and os.path.splitext(filename)[1].lower() == '.' + suffix:
            return True
        return False

    @classmethod
    def check_format(cls, filename):
        try:
            cls.input(filename)
            return True
        except Exception:
            return False
        return False

    @classmethod
    def input(cls, filename):
        #return []
        raise ValueError

    @classmethod
    def output(cls, datas):
        #return ''
        raise NotImplementedError
        
class FormatPbn(FormatParser):
    # http://webpbn.com/pbn_fmt.html
    name = 'pbn'
    suffix = 'xml'
    @classmethod
    def input(cls, filename):
        fp = file(filename, 'U')
        content = fp.read()
        if 'http://webpbn.com/pbn-0.3.dtd' not in content:
            raise ValueError
        datas = []
        data = None
        image = []
        color_char = ''
        for line in content.splitlines():
            if '<puzzle' in line:
                data = NonoData()
                image = []
            if line.startswith('|'):
                image.append(line[1:-1])
            m = re.match(r'^<color.*char="(.)"', line)
            if m:
                symbol = m.group(1)
                if symbol not in color_char:
                    color_char += symbol
                if len(color_char) > 2:
                    data.multicolor = True
                    return []
            m = re.match(r'^<note>(.*)</note>', line)
            if m:
                note = m.group(1)
                if 'definitely unique' in note:
                    data.unique = True
                elif 'definitely non-unique' in note:
                    data.unique = False
                else:
                    data.unique = True
                    #print filename, note
            if '</image>' in line:
                assert '.' in color_char
                if '.' in color_char: # reorder '.' to first
                    color_char = '.' + color_char.replace('.', '')
                if len(color_char) > 1:
                    c1 = color_char[1]
                    for i in range(len(image)):
                        image[i] = image[i].replace(c1, 'X')
                data.width = len(image[0])
                data.height = len(image)
                data.goal = image
                for row in image:
                    data.rows.append(map(len, re.findall('X+', row)))
                for col in zip(*image):
                    col = ''.join(col)
                    data.cols.append(map(len, re.findall('X+', col)))
                if not data.validate():
                    raise ValueError
                datas.append(data)
        return datas


    @classmethod
    def output(cls, datas):
        result = ''

        result += '<?xml version="1.0"?>\n'
        result += '<!DOCTYPE pbn SYSTEM "http://webpbn.com/pbn-0.3.dtd">\n'
        result += '<puzzleset>\n'
        for i, data in enumerate(datas):
            result += '<puzzle type="grid" defaultcolor="black">\n'
            result += '<id>#%d</id>\n' % (i+1)
            result += '<color name="white" char=".">fff</color>\n'
            result += '<color name="black" char="X">000</color>\n'

            result += '<clues type="columns">\n'
            for col in data.cols:
                result += '<line>' + ''.join('<count>%d</count>' % x for x in col) + '</line>\n'
            result += '</clues>\n'

            result += '<clues type="rows">\n'
            for row in data.rows:
                result += '<line>' + ''.join('<count>%d</count>' % x for x in row) + '</line>\n'
            result += '</clues>\n'

            result += '</puzzle>\n'
        result += '</puzzleset>\n'

        return result

class FormatNon(FormatParser):
    # http://www.comp.lancs.ac.uk/~ss/nonogram/fmt2
    name = 'non'
    suffix = 'non'
    @classmethod
    def input(cls, filename):
        data = NonoData()
        fp = file(filename)

        while True:
            line = fp.readline()
            if not line:
                break

            m = re.match(r'^width (\d+)', line)
            if m:
                data.width = int(m.group(1))
                continue

            m = re.match(r'^height (\d+)', line)
            if m:
                data.height = int(m.group(1))
                continue

            m = re.match(r'^rows\b', line)
            if m:
                assert data.height and data.width
                for j in range(data.height):
                    row = map(int, fp.readline().split(','))
                    if row == [0]:
                        row = []
                    data.rows.append(row)
                continue

            m = re.match(r'^columns\b', line)
            if m:
                assert data.height and data.width
                for j in range(data.width):
                    col = map(int, fp.readline().split(','))
                    if col == [0]:
                        col = []
                    data.cols.append(col)
                continue

            m = re.match(r'^goal\s"?([01]+)"?', line)
            if m:
                assert data.height and data.width
                goal = m.group(1)
                goal = goal.replace('0', '.').replace('1', 'X')
                assert len(goal) == data.width * data.height
                data.goal = []
                for i in range(data.height):
                    data.goal.append(goal[:data.width])
                    goal = goal[data.width:]
                assert goal == ''
                continue

            m = re.match(r'^goal\b', line)
            if m:
                assert data.height and data.width
                data.goal = []
                for i in range(data.height):
                    data.goal.append(fp.readline().strip().replace('0', '.').replace('1', 'X'))
                continue

        if not data.validate():
            raise ValueError
        return [data]

        raise NotImplementedError

    @classmethod
    def output(cls, datas):
        assert len(datas) == 1
        result = ''
        for data in datas:
            assert not data.multicolor
            result += 'width %d\n' % data.width
            result += 'height %d\n' % data.height

            result += '\n'
            result += 'rows\n'
            for row in data.rows:
                if row:
                    result += ','.join(map(str, row)) + '\n'
                else:
                    result += '0\n'

            result += '\n'
            result += 'columns\n'
            for col in data.cols:
                if col:
                    result += ','.join(map(str, col)) + '\n'
                else:
                    result += '0\n'

            if data.goal:
                result += '\n'
                result += 'goal\n'
                for row in data.goal:
                    result += row.replace('.', '0').replace('X', '1') + '\n'

        return result

class FormatRaw(FormatParser):
    # for rand30
    name = 'raw'
    suffix = ''
    @classmethod
    def input(cls, filename):
        fp = file(filename, 'U')
        content = fp.read()
        data = NonoData()
        image = []
        goal = []
        for line in content.splitlines():
            line = line.replace('1', 'X').replace('0', '.')
            image.append(line)
            goal.append(line)

        data.width = len(image[0])
        data.height = len(image)
        for row in image:
            data.rows.append(map(len, re.findall('X+', row)))
        for col in zip(*image):
            col = ''.join(col)
            data.cols.append(map(len, re.findall('X+', col)))
        data.goal = goal
        if not data.validate():
            raise ValueError
        return [data]


    @classmethod
    def output(cls, datas):
        result = ''

        result += '<?xml version="1.0"?>\n'
        result += '<!DOCTYPE pbn SYSTEM "http://webpbn.com/pbn-0.3.dtd">\n'
        result += '<puzzleset>\n'
        for i, data in enumerate(datas):
            assert not data.multicolor
            result += '<puzzle type="grid" defaultcolor="black">\n'
            result += '<id>#%d</id>\n' % (i+1)
            result += '<color name="white" char=".">fff</color>\n'
            result += '<color name="black" char="X">000</color>\n'

            result += '<clues type="columns">\n'
            for col in data.cols:
                result += '<line>' + ''.join('<count>%d</count>' % x for x in col) + '</line>\n'
            result += '</clues>\n'

            result += '<clues type="rows">\n'
            for row in data.rows:
                result += '<line>' + ''.join('<count>%d</count>' % x for x in row) + '</line>\n'
            result += '</clues>\n'

            result += '</puzzle>\n'
        result += '</puzzleset>\n'

        return result
class FormatNin(FormatParser):
    # http://jwilk.net/software/nonogram
    name = 'nin'
    suffix = 'nin'
    @classmethod
    def input(cls, filename):
        data = NonoData()
        fp = file(filename)
        data.width, data.height = map(int, fp.readline().split())

        for j in range(data.height):
            data.rows.append(map(int, fp.readline().split()))
        for j in range(data.width):
            data.cols.append(map(int, fp.readline().split()))
        if not data.validate():
            raise ValueError
        return [data]

    @classmethod
    def output(cls, datas):
        assert len(datas) == 1
        result = ''
        for data in datas:
            assert not data.multicolor
            result += '%d %d\n' % (data.width, data.height)

            for row in data.rows:
                result += ' '.join(map(str, row)) + '\n'
            for col in data.cols:
                result += ' '.join(map(str, col)) + '\n'
        return result

class FormatTCGA(FormatParser):
    name = 'tcga'
    @classmethod
    def input(cls, filename):
        ncase = 100
        n = 25
        fp = file(filename, 'U')
        datas = []
        for i in range(ncase):
            data = NonoData()
            data.width = data.height = n
            line = fp.readline().strip()
            if '$%d' % (i+1) != line:
                raise ValueError

            for j in range(n):
                data.cols.append(map(int, fp.readline().split()))
            for j in range(n):
                data.rows.append(map(int, fp.readline().split()))
            if not data.validate():
                raise ValueError
            datas.append(data)
        return datas

    @classmethod
    def output(cls, datas):
        ncase = 100
        #assert len(datas) == ncase
        n = 25
        for data in datas:
            assert not data.multicolor
            assert data.width == data.height == n


        result = ''
        for i, data in enumerate(datas):
            result += '$%d\n' % (i+1)
            for col in data.cols:
                result += '\t'.join(map(str, col)) + '\n'
            for row in data.rows:
                result += '\t'.join(map(str, row)) + '\n'

        return result

class FormatPycp(FormatParser):
    # for Google CP nonogram solver
    name = 'pycp'
    @classmethod
    def output(cls, datas):
        assert len(datas) == 1
        result = ''
        for data in datas:
            assert not data.multicolor
            result += 'rows = %d\n' % data.height
            row_rule_len = max(map(len, data.rows))
            result += 'row_rule_len = %d\n' % row_rule_len
            result += 'row_rules = [\n'
            for row in data.rows:
                row = [0]*row_rule_len + row
                row = row[-row_rule_len:]
                result += '\t[' + ','.join(map(str, row)) + '],\n'
            result += ']\n'

            result += '\n'

            result += 'cols = %d\n' % data.width
            col_rule_len = max(map(len, data.cols))
            result += 'col_rule_len = %d\n' % col_rule_len
            result += 'col_rules = [\n'
            for col in data.cols:
                col = [0]*col_rule_len + col
                col = col[-col_rule_len:]
                result += '\t[' + ','.join(map(str, col)) + '],\n'
            result += ']\n'
        return result

all_format = FormatNin, FormatTCGA, FormatPbn, FormatNon, FormatPycp, FormatRaw

def determine_reader(filename):
    reader = None
    if args.fromfmt == 'auto':
        for fmt in all_format:
            if fmt.check_format(filename):
                reader = fmt
                break
    else:
        for fmt in all_format:
            if fmt.name == args.fromfmt:
                reader = fmt
                break

    return reader


def determine_writer():
    writer = None
    for fmt in all_format:
        if fmt.name == args.tofmt:
            writer = fmt
            break

    return writer

def main():
    writer = determine_writer()
    assert writer

    datas = []
    for filename in args.filenames:
        reader = determine_reader(filename)
        assert reader

        for data in reader.input(filename):
            data.filename = filename
            datas.append(data)

    if args.filter_color == 'yes':
        datas = [ x for x in datas if x.multicolor ]
    if args.filter_color == 'no':
        datas = [ x for x in datas if not x.multicolor ]
    if args.filter_size:
        datas = [ x for x in datas if x.width <= args.filter_size and x.height <= args.filter_size ]
    if args.filter_square == 'yes':
        datas = [ x for x in datas if x.width == x.height ]
    if args.filter_square == 'no':
        datas = [ x for x in datas if x.width != x.height ]
    if args.filter_unique == 'yes':
        datas = [ x for x in datas if x.unique == True ]
    if args.filter_unique == 'no':
        datas = [ x for x in datas if x.unique == False ]

    if args.limit:
        datas = datas[:args.limit]

    if args.index != 0:
        assert 1 <= args.index <= len(datas)
        datas = [datas[args.index-1]]

    if args.outdir:
        assert writer.suffix
        for i, data in enumerate(datas):
            basename = os.path.splitext(os.path.basename(data.filename))[0]
            if args.renumber:
                basename += '-%03d' % (i + 1)
            sys.stderr.write('.')
            #print basename
            basename = '%s.%s' % (basename, writer.suffix)
            filename = os.path.join( args.outdir, basename)
            fp = file(filename, 'w')
            fp.write(writer.output([data]))
            fp.close()
        sys.stderr.write('\n')
    else:
        sys.stdout.write(writer.output(datas))

if __name__ == '__main__':
    main()
