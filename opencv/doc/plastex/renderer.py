import string, re
import sys
from plasTeX.Renderers import Renderer

# import generated OpenCV function names
# if the file function_names.py does not exist, it
# can be generated using the script find_function_names.sh
try:
    from function_names import opencv_function_names
except:
    opencv_function_names = []
    pass

class XmlRenderer(Renderer):
    
    def default(self, node):
        """ Rendering method for all non-text nodes """
        s = []

        # Handle characters like \&, \$, \%, etc.
        if len(node.nodeName) == 1 and node.nodeName not in string.letters:
            return self.textDefault(node.nodeName)

        # Start tag
        s.append('<%s>' % node.nodeName)

        # See if we have any attributes to render
        if node.hasAttributes():
            s.append('<attributes>')
            for key, value in node.attributes.items():
                # If the key is 'self', don't render it
                # these nodes are the same as the child nodes
                if key == 'self':
                    continue
                s.append('<%s>%s</%s>' % (key, unicode(value), key))
            s.append('</attributes>')

        # Invoke rendering on child nodes
        s.append(unicode(node))

        # End tag
        s.append('</%s>' % node.nodeName)

        return u'\n'.join(s)

    def textDefault(self, node):
        """ Rendering method for all text nodes """
        return node.replace('&','&amp;').replace('<','&lt;').replace('>','&gt;')

from plasTeX.Renderers import Renderer as BaseRenderer

class reStructuredTextRenderer(BaseRenderer):

  aliases = {
        'superscript': 'active::^',
        'subscript': 'active::_',
        'dollar': '$',
        'percent': '%',
        'opencurly': '{',
        'closecurly': '}',
        'underscore': '_',
        'ampersand': '&',
        'hashmark': '#',
        'space': ' ',
        'tilde': 'active::~',
        'at': '@',
        'backslash': '\\',
  }

  def __init__(self, *args, **kwargs):
    BaseRenderer.__init__(self, *args, **kwargs)

    # Load dictionary with methods
    for key in vars(type(self)):
      if key.startswith('do__'):
        self[self.aliases[key[4:]]] = getattr(self, key)
      elif key.startswith('do_'):
        self[key[3:]] = getattr(self, key)

    self.indent = 0
    self.in_func = False
    self.in_cvarg = False
    self.descriptions = 0
    self.after_parameters = False
    self.func_short_desc = ''

  def do_document(self, node):
    return unicode(node)

  def do_par(self, node):
    if self.indent == -1:
      pre = ""
      post = ""
    else:
      pre = "\n" + (" " * self.indent)
      post = "\n"
    return pre + unicode(node).lstrip(" ") + post

  def do_chapter(self, node):
    t = str(node.attributes['title'])

    section_files = []
    for section in node.subsections:
        try:
            filename = section.filenameoverride
            if filename is not None:
                section_files.append(filename)
        except:
            pass

    toc = ".. toctree::\n   :maxdepth: 2\n\n"
    for file in section_files:
        if file[-4:] != '.rst':
            print >>sys.stderr, "WARNING: unexpected file extension:", file
        else:
            toc += "   %s\n" % file[:-4]
    toc += "\n\n"

    return "\n\n%s\n%s\n%s\n\n" % ('*' * len(t), t, '*' * len(t)) + toc + unicode(node)

  def do_section(self, node):
    t = str(node.attributes['title'])
    return "\n\n%s\n%s\n\n" % (t, '=' * len(t)) + unicode(node)

  def do_subsection(self, node):
    t = str(node.attributes['title'])
    return "\n\n%s\n%s\n\n" % (t, '-' * len(t)) + unicode(node)

  def do_cvexp(self, node):
    self.indent = -1
    self.in_func = False
    decl = unicode(node.attributes[self.language]).rstrip(' ;')  # remove trailing ';'
    r = u"\n\n.. %s:: %s\n\n" % ({'c' : 'cfunction', 'py' : 'function'}[self.language], decl)
    self.indent = 4
    if self.func_short_desc != '':
      r += self.ind() + self.func_short_desc + '\n\n'
      self.func_short_desc = ''
    return r

  def do_description(self, node):
    self.descriptions += 1
    desc = unicode(node)
    self.descriptions -= 1
    if self.descriptions == 0:
      self.after_parameters = True
    return u"\n\n" + desc + u"\n\n"

  def do_includegraphics(self, node):
    filename = '../' + str(node.attributes['file']).strip()
    if not os.path.isfile(filename):
        print >>sys.stderr, "WARNING: missing image file", filename
        return u""
    return u"\n\n%s.. image:: %s\n\n" % (self.ind(), filename)

  def do_cvfunc(self, node):
    t = unicode(node.attributes['title']).strip()
    print "====>", t
    label = u"\n\n.. index:: %s\n\n.. _%s:\n\n" % (t, t)
    self.in_func = True
    self.descriptions = 0
    self.after_parameters = False
    self.indent = 0
    return u"" + unicode(node)

    # Would like to look ahead to reorder things, but cannot see more than 2 ahead
    if 0:
      print "NODES:", node.source
      n = node.nextSibling
      while (n != None) and (n.nodeName != 'cvfunc'):
        print "   ", n.nodeName, len(n.childNodes)
        n = n.nextSibling
      print "-----"
    return label + u"\n\n%s\n%s\n\n" % (t, '^' * len(t)) + unicode(node)

  def do_cvstruct(self, node):
    t = str(node.attributes['title']).strip()
    self.after_parameters = False
    self.indent = 4
    return u".. ctype:: %s" % t + unicode(node)

  def do_cvmacro(self, node):
    t = str(node.attributes['title']).strip()
    self.after_parameters = False
    self.indent = 4
    return u".. cmacro:: %s" % t + unicode(node)

  def showTree(self, node, i = 0):
    n = node
    while n != None:
      print "%s[%s]" % (" " * i, n.nodeName)
      if len(n.childNodes) != 0:
        self.showTree(n.childNodes[0], i + 4)
      n = n.nextSibling

  def do_Huge(self, node):
    return unicode(node)

  def do_tabular(self, node):
    if 0:
      self.showTree(node)
    rows = []
    for row in node.childNodes:
      cols = []
      for col in row.childNodes:
        cols.append(unicode(col).strip())
      rows.append(cols)
    maxes = [ 0 ] * len(rows[0])
    for r in rows:
      maxes = [ max(m,len(c)) for m,c in zip(maxes, r) ]
    sep = "+" + "+".join([ ('-' * (m + 4)) for m in maxes]) + "+"
    s = ""
    s += sep + "\n"
    for r in rows:
      s += "|" + "|".join([ ' ' + c.ljust(m + 3) for c,m in zip(r, maxes) ]) + "|" + "\n"
      s += sep + "\n"
    return unicode(s)

  def do_verbatim(self, node):
    return u"\n\n::\n\n    " + unicode(node.source.replace('\n', '\n    ')) + "\n\n"

  def do_index(self, node):
    return u""
    # No idea why this does not work... JCB
    return u"\n\n.. index:: (%s)\n\n" % node.attributes['entry']

  def do_label(self, node):
    return u""

  def fixup_funcname(self, str):
    """
    add parentheses to a function name if not already present
    """
    str = str.strip()
    if str[-1] != ')':
      return str + '()'
    return str

  def gen_reference(self, name):
    """
    try to guess whether *name* is a function, struct or macro
    and if yes, generate the appropriate reference markup
    """
    name = name.strip()
    if name[0:2] == 'cv':
        return u":cfunc:`%s`" % self.fixup_funcname(name)
    elif 'cv'+name in opencv_function_names:
        if self.language in ['c', 'cpp']:
            return u":cfunc:`cv%s`" % self.fixup_funcname(name)
        else:
            return u":func:`%s`" % self.fixup_funcname(name)
    elif name[0:2] == 'Cv' or name[0:3] == 'Ipl':
        return u":ctype:`%s`" % name
    elif name[0:2] == 'CV':
        return u":cmacro:`%s`" % name
    return None

  def do_cross(self, node):
    t = str(node.attributes['name']).strip()
    # try to guess whether t is a function, struct or macro
    # and if yes, generate the appropriate reference markup
    rst_ref = self.gen_reference(t)
    if rst_ref is not None:
        return rst_ref
    return u":ref:`%s`" % str(node.attributes['name']).strip()

  def ind(self):
    return u" " * self.indent

  def do_cvarg(self, node):
    self.indent += 4

    print "HELLO", str(node.attributes['item']).strip()
    # Nested descriptions occur e.g. when a flag parameter can 
    # be one of several constants.  We want to render the inner 
    # description differently than the outer parameter descriptions.
    if self.in_cvarg or self.after_parameters:
      defstr = unicode(node.attributes['def'])
      assert not (u"\xe2" in unicode(defstr))
      self.indent -= 4
      param_str = u"\n%s  * **%s** - %s\n" 
      return param_str % (self.ind(), str(node.attributes['item']).strip(), self.fix_quotes(defstr).strip(" "))

    # save that we are in a paramater description
    self.in_cvarg = True
    defstr = unicode(node.attributes['def'])
    assert not (u"\xe2" in unicode(defstr))
    self.in_cvarg = False

    self.indent -= 4
    param_str = u"\n%s:param %s: %s"
    return param_str % (self.ind(), str(node.attributes['item']).strip(), self.fix_quotes(defstr).strip())


  def do_bgroup(self, node):
    return u"bgroup(%s)" % node.source

  def do_url(self, node):
    return unicode(node.attributes['loc'])

  def do_enumerate(self, node):
    return unicode(node)

  def do_itemize(self, node):
    return unicode(node)

  def do_item(self, node):
    if node.attributes['term'] != None:
      self.indent += 4
      defstr = unicode(node).strip()
      assert not (u"\xe2" in unicode(defstr))
      self.indent -= 4
      return u"\n%s* %s *\n%s    %s\n" % (self.ind(), unicode(node.attributes['term']).strip(), self.ind(), defstr)
    else:
      return u"\n\n%s* %s" % (self.ind(), unicode(node).strip())

  def do_textit(self, node):
    return "*%s*" % unicode(node.attributes['self'])

  def do_texttt(self, node):
    t = unicode(node)
    # try to guess whether t is a function, struct or macro
    # and if yes, generate the appropriate reference markup
    rst_ref = self.gen_reference(t)
    if rst_ref is not None:
        return rst_ref
    return u"``%s``" % t

  def do__underscore(self, node):
    return u"_"

  def default(self, node):
    print "DEFAULT dropping", node.nodeName
    return unicode(node)

  def do_lstlisting(self, node):
    self.in_func = False
    lines = node.source.split('\n')
    body = "\n".join([u"%s    %s" % (self.ind(), s) for s in lines[1:-1]])
    return u"\n\n%s::\n\n" % self.ind() + unicode(body) + "\n\n"

  def do_math(self, node):
    return u":math:`%s`" % node.source

  def do_displaymath(self, node):
    words = self.fix_quotes(node.source).strip().split()
    return u"\n\n%s.. math::\n\n%s   %s\n\n" % (self.ind(), self.ind(), " ".join(words[1:-1]))

  def do_maketitle(self, node):
    return u""
  def do_setcounter(self, node):
    return u""
  def do_tableofcontents(self, node):
    return u""
  def do_titleformat(self, node):
    return u""
  def do_subsubsection(self, node):
    return u""
  def do_include(self, node):
    return u""

  def fix_quotes(self, s):
    s = s.replace(u'\u2013', "'")
    s = s.replace(u'\u2019', "'")
    s = s.replace(u'\u2264', "#<2264>")
    s = s.replace(u'\xd7', "#<d7>")
    return s

  def do_ifthenelse(self, node):
    # print "IFTHENELSE: [%s],[%s],[%s]" % (node.attributes['test'], str(node.attributes['then']), node.attributes['else'])
    print "CONDITION", unicode(node.attributes['test']).strip() == u'true'
    if unicode(node.attributes['test']).strip() == u'true':
      print "TRUE: [%s]" % str(node.attributes['then'])
      return unicode(node.attributes['then'])
    else:
      return unicode(node.attributes['else'])

  def do_equal(self, node):
    first = unicode(node.attributes['first']).strip()
    second = unicode(node.attributes['second']).strip()
    if first == second:
      return u'true'
    else:
      return u'false'

  def textDefault(self, node):
    if self.in_func:
      self.func_short_desc += self.fix_quotes(unicode(node)).strip(" ")
      return u""

    s = unicode(node)
    s = self.fix_quotes(s)
    return s
    return node.replace('\\_','_')


from plasTeX.TeX import TeX
import os
import pickle

def parse_documentation_source(language):
    # Instantiate a TeX processor and parse the input text
    tex = TeX()
    tex.ownerDocument.config['files']['split-level'] = 0
    #tex.ownerDocument.config['files']['filename'] = 'cxcore.rst'

    if 1:
        tex.input("\\input{online-opencv-%s.tex}" % language)
    else:
        src0 = r'''
        \documentclass{book}
        \usepackage{myopencv}
        \begin{document}'''

        src1 = r'''
        \end{document}
        '''
        lines = list(open("../CvReference.tex"))
        LINES = 80
        tex.input(src0 + "".join(lines[:LINES]) + src1)

    return tex.parse()

language = sys.argv[1]
document = parse_documentation_source(language)

rest = reStructuredTextRenderer()
rest.language = language
rest.render(document)
