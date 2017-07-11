'''
Input: This program reads one or more SVG files generated by InkScape and searches for "paths" containing:
    - lane width,
    - sequence of points centered along a road lane,
    - left and right lane markings according to a color code.
The paths are then expanded to a continuum of points according to the cubic Bezier curve algorithm.
Output: The road lane map is generated for all points within the lane width, setting their probabilities of being:
    - off the road,
    - on a solid line marking,
    - on a broken line marking, 
    - at the center of a lane. 
'''
import sys
import numpy as np
import cv2
import struct
from xml.dom import minidom
import time
import os

# Global definitions
VERBOSE = 0
ANIMATION = 0
BEZIER_FRACTION = 0.001     # Line length increment (from 0.000 to 1.000) to set cubic Bezier curve points (number of points = 1/fraction) 
NO_MARKING = 0
BROKEN_LINE = 1
SOLID_LINE = 2
MAX_PROB = (2.0**16 - 1.0)          # Probability maximum value (unsigned short)
LOCAL_GRIDMAP_RESOLUTION = 0.2      # Map pixel dimension in meters: grid_mapping/grid_mapping.c
CARMEN_MAP_LABEL = "CARMENMAPFILE"  # maptools/map_io.h
CARMEN_MAP_VERSION = "v020"         # maptools/map_io.h
CARMEN_MAP_CREATOR_CHUNK = 32       # maptools/map_io.h
CARMEN_MAP_GRIDMAP_CHUNK = 1        # maptools/map_io.h

#https://docs.python.org/2/library/struct.html
class road:
    def __init__(self):
        self.off_the_road = int(MAX_PROB)   # unsigned short: 'H'
        self.solid_marking = 0              # unsigned short: 'H'
        self.broken_marking = 0             # unsigned short: 'H'
        self.lane_center = 0                # unsigned short: 'H'
        self.lane_number = 0
        self.in_the_lane = False

#https://developer.mozilla.org/en-US/docs/Web/SVG/Tutorial/Paths
def svg_d_get_bezier_points(d):
    letter = ''
    count = 0
    points = [] # list of (x,y)
    last_abs_point_i = 0
    errors = 0
    n_ms = 0
    for p in d.split(' '):    
        if len(p) == 1:
            if p == 'm' or p == 'M' or p == 'c' or p == 'C' or p == 'l' or p == 'L': # or p == 'h' or p == 'H' or p == 'v' or p == 'V':
                if p != letter:
                    count = 0
                letter = p
            else:
                errors += 1
        else:  
            if letter == 'm' or letter == 'M': # Move cursor to (x,y), lowercase = relative coordinates, uppercase = absolute coordinates
                pt = p.split(',')
                if n_ms > 0: # In case we have more than 1 (m or M)
                    points.append(points[-1]) # Repeat last point just to fake the Bezier algorithm
                    if letter == 'm':
                        # calculate absolute point
                        delta = pt
                        points.append((float(delta[0]) + points[-1][0], float(delta[1]) + points[-1][1]))
                    else:
                        points.append((float(pt[0]), float(pt[1])))
                    points.append(points[-1]) # Repeat new point just to fake the Bezier algorithm
                    last_abs_point_i = len(points) - 1
                else:
                    if len(pt) == 2:
                        points.append((float(pt[0]), float(pt[1])))
                        n_ms += 1
                    else:
                        errors += 1             
            elif letter == 'c': # Cubic Bezier curve, lowercase = relative coordinates
                count += 1   
                delta = p.split(',')
                # calculate absolute point
                if len(delta) == 2:                
                    pt[0] = float(delta[0]) + points[last_abs_point_i][0]
                    pt[1] = float(delta[1]) + points[last_abs_point_i][1]
                    points.append((float(pt[0]), float(pt[1])))     
                    if count % 3 == 0:
                        last_abs_point_i = len(points) - 1
                else:
                    errors +=1                            
            elif letter == 'C': # Cubic Bezier curve, uppercase = absolute coordinates
                count += 1            
                pt = p.split(',') 
                if len(pt) == 2:
                    points.append((float(pt[0]), float(pt[1])))
                    last_abs_point_i = len(points) - 1
                else:
                    errors +=1                            
            elif letter == 'l': # Draw straight line to next point (x,y), lowercase = relative coordinates
                delta = p.split(',')
                # calculate absolute point
                if len(delta) == 2:                
                    points.append(points[-1]) # Repeat last point just to fake the Bezier algorithm
                    pt[0] = float(delta[0]) + points[last_abs_point_i][0]
                    pt[1] = float(delta[1]) + points[last_abs_point_i][1]
                    points.append((float(pt[0]), float(pt[1])))
                    last_abs_point_i = len(points) - 1
                    points.append(points[-1]) # Repeat new point just to fake the Bezier algorithm
                else:
                    errors += 1             
            elif letter == 'L': # Draw straight line to next point (x,y), uppercase = absolute coordinates
                pt = p.split(',') 
                if len(pt) == 2:                
                    points.append(points[-1]) # Repeat last point just to fake the Bezier algorithm
                    points.append((float(pt[0]), float(pt[1])))
                    last_abs_point_i = len(points) - 1
                    points.append(points[-1]) # Repeat new point just to fake the Bezier algorithm
                else:
                    errors += 1             
            else:
                errors += 1
        if errors == 1:
            print 'Error in SVG line: d="', d, '"'
        if errors >= 1:
            print 'Unexpected SVG token: ', p
    return points

#https://stackoverflow.com/questions/15857818/python-svg-parser
def svg_get_paths(svg_file):
    doc = minidom.parse(svg_file)  # parseString also exists    
    paths = []
    img = doc.getElementsByTagName('image')
    width = int(img[0].getAttribute('width'))
    height = int(img[0].getAttribute('height'))
    for path in doc.getElementsByTagName('path'):
        d = path.getAttribute('d')
        points = svg_d_get_bezier_points(d)
        for style in path.getAttribute('style').split(';'):
            s = style.split(':')
            if s[0] == 'stroke-width':
                stroke_width = float(s[1]) 
            if s[0] == 'stroke': # this field has the stroke color
                # stroke-color codes the lane marking according to readme.txt
                stroke_color = s[1]
        paths.append((points, stroke_width, stroke_color))
    doc.unlink()
    return width, height, paths

def normalize(a, b):
    norm = np.sqrt(a * a + b * b)
    if norm == 0.0:
        return a, b
    return (a / norm), (b / norm)

#https://stackoverflow.com/questions/785097/how-do-i-implement-a-b%C3%A9zier-curve-in-c
#https://stackoverflow.com/questions/37642168/how-to-convert-quadratic-bezier-curve-code-into-cubic-bezier-curve
def getPt(n1, n2 , perc):
    diff = n2 - n1;
    return n1 + ( diff * perc )

def get_bezier(width, height, lane, fraction, points):
    # width, heigh (image dimensions): used for adjustment of point coordinates
    # lane number in SVG file
    # fraction = 1 / number of real points each Bezier segment generates 
    # points: Bezier polyline parameters
    bx = []
    by = []
    bxo = []
    byo = []
    for i in range((len(points)-1)/3):
        j = 0.0
        while j <= 1.0:
            # The Green Lines
            xa = getPt(points[i*3][0], points[i*3+1][0], j)
            ya = getPt(points[i*3][1], points[i*3+1][1], j)
            xb = getPt(points[i*3+1][0], points[i*3+2][0], j)
            yb = getPt(points[i*3+1][1], points[i*3+2][1], j)
            xc = getPt(points[i*3+2][0], points[i*3+3][0], j)
            yc = getPt(points[i*3+2][1], points[i*3+3][1], j)
            # The Blue Line
            xm = getPt(xa, xb, j)
            ym = getPt(ya, yb, j)
            xn = getPt(xb, xc, j)
            yn = getPt(yb, yc, j)    
            # The Black Dot
            x = getPt(xm, xn, j)
            y = getPt(ym, yn, j)      
            bx.append(x)
            by.append(height - y) # Image's Y axis orientation is downwards, but IARA's orientation is upwards
            if len(bx) == 2:
                last_byo, last_bxo = normalize(by[1] - by[0], bx[1] - bx[0])
                bxo.append(last_bxo)
                byo.append(last_byo)
            if len(bx) >= 3:
                next_byo, next_bxo = normalize(by[-1] - by[-2], bx[-1] - bx[-2])
                avg_byo, avg_bxo = normalize((last_byo + next_byo) / 2.0, (last_bxo + next_bxo) / 2.0)
                last_bxo = next_bxo
                last_byo = next_byo
                bxo.append(avg_bxo)
                byo.append(avg_byo)
            j += fraction
    if len(bx) < 2:
        print 'Invalid Bezier curve in lane number', lane,': len(points) =', len(points), ', len(bx) =', len(bx), ', len(by) =', len(by)
        return [], [], [], []
    bxo.append(last_bxo)
    byo.append(last_byo)
    return bx, by, bxo, byo

#     # Consolidate Bezier points into image pixels
#     bx2 = []
#     by2 = []
#     bxo2 = []
#     byo2 = []
#     last_x = int(round(bx[0]))
#     last_y = int(round(by[0]))
#     sum_bxo = bxo[0]
#     sum_byo = byo[0]
#     count = 1
#     for i in range(1, len(bx)):
#         x = int(round(bx[i]))
#         y = int(round(by[i]))
#         if x != last_x or y != last_y:
#             avg_bxo = sum_bxo / count
#             avg_byo = sum_byo / count
#             bx2.append(last_x)
#             by2.append(last_y)
#             bxo2.append(avg_bxo)
#             byo2.append(avg_byo)
#             last_x = x
#             last_y = y
#             sum_bxo = bxo[i]
#             sum_byo = byo[i]
#             count = 1
#         else:
#             sum_bxo += bxo[i]
#             sum_byo += byo[i]
#             count += 1
#     avg_bxo = sum_bxo / count
#     avg_byo = sum_byo / count
#     bx2.append(last_x)
#     by2.append(last_y)
#     bxo2.append(avg_bxo)
#     byo2.append(avg_byo)
#     return bx2, by2, bxo2, byo2

def scalar_product(dx1, dy1, dx2, dy2):
    d_cos = dx1 * dx2 + dy1 * dy2
    d_sin = dy1 * dx2 - dx1 * dy2
    return d_cos, d_sin

def distance_to_line(x, y, bx, by, bxo, byo):
    # Perform a binary search for a dot product (scalar product) near zero (orthogonal vectors)
    # Scalar product of vectors A and B is ||A||.||B||.cos(a' - b') = ||A||.||B||.(cos a'.cos b' + sen a'.sen b')
    # A = vector (bx, by) - (x, y)
    # B = unit vector (bxo, byo)
    i0 = 0
    dx0 = bx[i0] - x
    dy0 = by[i0] - y
    dbx0 = bxo[i0]
    dby0 = byo[i0]
    dbc0, dbs0 = scalar_product(dx0, dy0, dbx0, dby0)
    i1 = len(bx) - 1
    dx1 = bx[i1] - x
    dy1 = by[i1] - y
    dbx1 = bxo[i1]
    dby1 = byo[i1]
    dbc1, dbs1 = scalar_product(dx1, dy1, dbx1, dby1)
    if VERBOSE >= 3: print '\ti0 =', i0, ' i1 =', i1, ' dbc0 =', dbc0, ' dbs0 =', dbs0, 'dbc1 =', dbc1, ' dbs1 =', dbs1, \
                           ' bx0 =', bx[i0],' by0 =', by[i0], ' bx1 =', bx[i1],' by1 =', by[i1], \
                           ' dx0 =', dx0, ' dy0 =', dy0, ' dbx0 =', dbx0, ' dby0 =', dby0, \
                           ' dx1 =', dx1, ' dy1 =', dy1, ' dbx1 =', dbx1, ' dby1 =', dby1    
    while (i1 - i0) > 1 and np.signbit(dbc1) != np.signbit(dbc0):
        i2 = int((i0 + i1) / 2)
        dx2 = bx[i2] - x
        dy2 = by[i2] - y
        dbx2 = bxo[i2]
        dby2 = byo[i2]
        dbc2, dbs2 = scalar_product(dx2, dy2, dbx2, dby2)
        if np.signbit(dbc2) != np.signbit(dbc0):
            i1 = i2
            dbc1 = dbc2
            dbs1 = dbs2
        else:
            i0 = i2    
            dbc0 = dbc2
            dbs0 = dbs2
        if VERBOSE >= 3: print '\ti0 =', i0, ' i1 =', i1, ' dbc0 =', dbc0, ' dbs0 =', dbs0, 'dbc1 =', dbc1, ' dbs1 =', dbs1, \
                               ' bx0 =', bx[i0],' by0 =', by[i0], ' bx1 =', bx[i1],' by1 =', by[i1], \
                               ' i2 =', i2, ' dx2 =', dx2, ' dy2 =', dy2, ' dbx2 =', dbx2, ' dby2 =', dby2, ' dbc2 =', dbc2, ' dbs2 =', dbs2    
    if abs(dbc1) < abs(dbc0):
        i0 = i1
        dbc0 = dbc1
        dbs0 = dbs1
    d = np.sqrt((bx[i0] - x) * (bx[i0] - x) + (by[i0]- y) * (by[i0] - y))  
    return d, dbc0, dbs0, bxo[i0], byo[i0]

def get_lane_marking_by_color_code(stroke_color):
    if stroke_color == '#ff0000':
        l_marking = SOLID_LINE
        r_marking = SOLID_LINE 
    elif stroke_color == '#ff007f':
        l_marking = BROKEN_LINE
        r_marking = SOLID_LINE 
    elif stroke_color == '#7f00ff':
        l_marking = SOLID_LINE
        r_marking = BROKEN_LINE 
    elif stroke_color == '#0000ff':
        l_marking = BROKEN_LINE
        r_marking = BROKEN_LINE
    elif stroke_color == '#00ff00':
        l_marking = NO_MARKING
        r_marking = NO_MARKING
    elif stroke_color == '#ff7f00':
        l_marking = NO_MARKING
        r_marking = SOLID_LINE
    elif stroke_color == '#7fff00':
        l_marking = SOLID_LINE
        r_marking = NO_MARKING
    elif stroke_color == '#007fff':
        l_marking = NO_MARKING
        r_marking = BROKEN_LINE
    elif stroke_color == '#00ff7f':
        l_marking = BROKEN_LINE
        r_marking = NO_MARKING
    else:
        l_marking = NO_MARKING
        r_marking = NO_MARKING
    return l_marking, r_marking

def get_lane_from_bezier(map, bx, by, bxo, byo, lane, stroke_width, stroke_color, image, image_name):
    # map of lanes in a grid of pixels
    # bx, by: Bezier curve points
    # bxo, byo: Bezier curve points orientation
    # lane number in SVG file
    # stroke width of the lane
    # stroke color of the lane
    # image for displaying the animation
    height = len(map)
    width = len(map[0])
    max_distance = stroke_width / 2.0
    l_marking, r_marking = get_lane_marking_by_color_code(stroke_color)
    last_x = last_y = np.NaN
    cont = 0
    for i in range(len(bx)):
        x = int(round(bx[i]))
        y = int(round(by[i]))
        if x == last_x and y == last_y:
            if VERBOSE >= 2: print 'i =', i, ': Current and previous Bezier points are located in the same map pixel: x =', x, ', y =', y, ', bx =', bx[i], ', by =', by[i]
            continue
        last_x = x
        last_y = y
        neighbors = [(x, y)]
        while len(neighbors) > 0:
            x, y = neighbors.pop()
            if x < 0 or x >= width or y < 0 or y >= height:
                if VERBOSE >= 2: print 'i =', i, ': Pixel coordinates are out of map limits: x =', x, ', y =', y, ', width =', width, ', height =', height
                continue
            if map[y][x].lane_number == lane:
                if VERBOSE >= 2: print 'i =', i, ': Map pixel has already been analyzed by current lane number', lane, ': x =', x, ', y =', y
                continue
            if map[y][x].in_the_lane:
                if VERBOSE >= 2: print 'i =', i, ': Map pixel is already inside lane number', map[y][x].lane_number, ': x =', x, ', y =', y
                continue
            cont += 1
            if VERBOSE >= 3: print 'i =', i, ': distance_to_line call #', cont, ': x =', x, ', y =', y  
            d, dbcos, dbsin, dbx, dby = distance_to_line(x, y, bx, by, bxo, byo)
            map[y][x].lane_number = lane
            if d > max_distance:
                if VERBOSE >= 2: print 'i =', i, ': Map pixel is out of the current lane number', lane, ': d =', d, ', max_distance =', max_distance, ': x =', x, ', y =', y
                continue
            if d > 1.0 and abs(dbcos) > 1.0:
                if VERBOSE >= 2: print 'i =', i, ': Map pixel is not orthogonal to the current lane number', lane, ': d =', d, ': dbcos =', dbcos, ', arccos =', np.arccos(dbcos / d), ': x =', x, ', y =', y
                continue
            map[y][x].off_the_road = 0
            marking_line_prob = int(round((int((max_distance - d) < 1.0) + int((max_distance - d) < 2.0)) * 0.5 * MAX_PROB)) # probability in (0.0, 0.5, 1.0) * MAX_PROB
            if np.signbit(dbsin): # left side
                map[y][x].solid_marking = (l_marking == SOLID_LINE) * marking_line_prob
                map[y][x].broken_marking = (l_marking == BROKEN_LINE) * marking_line_prob
            else: # right side
                map[y][x].solid_marking = (r_marking == SOLID_LINE) * marking_line_prob
                map[y][x].broken_marking = (r_marking == BROKEN_LINE) * marking_line_prob
            map[y][x].lane_center = int(round((1.0 - 0.75 * d / max_distance) * MAX_PROB))  # probability in range(0.25, 1.0) * MAX_PROB
            map[y][x].in_the_lane = True 
            if VERBOSE >= 1: print 'i =', i, ': lane =', lane, ', x =', x, ', y =', y, ': d =', d, 'pixels, max_distance =', max_distance,  \
                                   ', lane_center =', map[y][x].lane_center, ', dbcos = ', dbcos, ', dbsin =', dbsin, \
                                   ', dbx =', dbx, ', dby =', dby, ', orientation =', np.arctan2(dby, dbx) / np.pi * 180, 'degrees', \
                                   ', left_marking =', l_marking, ', right_marking =', r_marking, ', marking_line_prob =', marking_line_prob, \
                                   ', solid_marking =', map[y][x].solid_marking,  ', broken_marking =', map[y][x].broken_marking
            for dy in (-1, 0, 1):   # Analyze the neighbor pixels
                for dx in (-1, 0, 1):
                    neighbors.append((x + dx, y + dy))
                    if VERBOSE >= 2: print '\tNeighbor pixel added: (x + dx) =', x + dx, ', (y + dy) =', y + dy
            if ANIMATION > 0:
                image[height - 1 - y][x] = (255, 255, 255)
                cv2.imshow(image_name, image)
                cv2.waitKey(ANIMATION)
    return map

def map_get_comment_chunk(size_x, size_y, resolution, origin, description):
    comment_chunk  = "#####################################################\n"
    comment_chunk += "#\n"
    comment_chunk += "# Carnegie Mellon Robot Toolkit (CARMEN) map file\n"
    comment_chunk += "#\n"
    comment_chunk += "# Map author    : R.Carneiro & R.Nascimento\n"
    comment_chunk += "# Creation date : %s\n" % (time.asctime())
    comment_chunk += "# Map size      : %d x %d\n" % (size_x, size_y)
    comment_chunk += "# Resolution    : %.1f\n" % (resolution)
    comment_chunk += "# Origin        : %s\n" % (origin)
    comment_chunk += "# Description   : %s\n" % (description)
    comment_chunk += "#\n"
    comment_chunk += "#####################################################\n"
    return comment_chunk

def map_get_id():
    return "%s%s" % (CARMEN_MAP_LABEL, CARMEN_MAP_VERSION)
 
def vchunk_size(chunk_type, *ap):
    size = 10       # chunk description
    if chunk_type == CARMEN_MAP_CREATOR_CHUNK:
        size += 10 + 8 + 80 + 80
    elif chunk_type == CARMEN_MAP_GRIDMAP_CHUNK:
        size += 12 + ap[0] * ap[1] * 4
    return size
 
def map_get_creator_chunk(origin, description):
    creator_chunk = chr(CARMEN_MAP_CREATOR_CHUNK)
    size = vchunk_size(CARMEN_MAP_CREATOR_CHUNK)
    creator_chunk += struct.pack('I', size)     # unsigned int: 'I'
    creator_chunk += "CREATOR   " 
    login = os.getlogin()
    if len(login) == 0:
      login = "UNKNOWN"
    login += (' ' * 10)
    creator_chunk += login[0:10]
    t = int(time.time())
    creator_chunk += struct.pack('Q', t)        # unsigned long long: 'Q'
    origin += (' ' * 80)
    creator_chunk += origin[0:80]
    description += (' ' * 80)
    creator_chunk += description[0:80]
    return creator_chunk

def map_get_gridmap_chunk(size_x, size_y, resolution):
    gridmap_chunk = chr(CARMEN_MAP_GRIDMAP_CHUNK)
    size = vchunk_size(CARMEN_MAP_GRIDMAP_CHUNK, size_x, size_y)
    gridmap_chunk += struct.pack('I', size)         # unsigned int: 'I'
    gridmap_chunk += "GRIDMAP   "
    gridmap_chunk += struct.pack('I', size_x)       # unsigned int: 'I'
    gridmap_chunk += struct.pack('I', size_y)       # unsigned int: 'I'
    gridmap_chunk += struct.pack('d', resolution)   # double: 'd'
    return gridmap_chunk

def map_write(map, svg_file, width, height):
    road_file = open('r' + svg_file[1:-3] + 'map', 'wb')
    comment_chunk = map_get_comment_chunk(width, height, LOCAL_GRIDMAP_RESOLUTION, "", "Generated by road_mapper_generate_gt3.py")
    road_file.write(comment_chunk)
    map_id = map_get_id()
    road_file.write(map_id)
    creator_chunk = map_get_creator_chunk("", "Generated by road_mapper_generate_gt3.py")
    road_file.write(creator_chunk)
    gridmap_chunk = map_get_gridmap_chunk(width, height, LOCAL_GRIDMAP_RESOLUTION)
    road_file.write(gridmap_chunk)
    for x in range(width):
        for y in range(height):
            m = map[y][x]
            road_file.write(struct.pack('HHHH', m.off_the_road, m.solid_marking, m.broken_marking, m.lane_center))
            
if __name__ == "__main__":
    USAGE = '[(-v|--verbose)=<level>] [(-a|--animation)=<milliseconds>] [(-f|--file)=<SVG file list>] [<SVG filename> ...]'
    filelists = []
    filenames = []
    for i in range(1, len(sys.argv)):
        opt = sys.argv[i].split('=')
        if opt[0] == '-h' or opt[0] == '--help':
            print 'Usage: python', sys.argv[0], USAGE
        elif opt[0] == '-v' or opt[0] == '--verbose':
            VERBOSE = int(opt[1])
            print 'Verbose option set to level', VERBOSE
        elif opt[0] == '-a' or opt[0] == '--animation':
            ANIMATION = int(opt[1])
            print 'Animation option set to', ANIMATION, 'milliseconds'
        elif opt[0] == '-f' or opt[0] == '--file':
            filelists.append(opt[1])
        elif opt[0][0] == '-':
            print 'Usage: python', sys.argv[0], USAGE
            print 'Unrecognized command line argument', sys.argv[i] 
        else:
            filenames.append(opt[0])
#     svg_file =  'i7726110_-353570.00.svg'
    svg_file =  'i7705600_-338380.00.svg'
    if len(filenames) > 0:
        svg_file = filenames[0]
    img_file =  svg_file[0:-3] + 'png'
    print 'Processing SVG file:', svg_file
    width, height, paths = svg_get_paths(svg_file)
    img1 = np.zeros((height, width, 3), np.uint8)
    img2 = np.zeros((height, width, 3), np.uint8)
    img_name1 = "cubic Bezier curve"
    img_name2 = "road map"
    cv2.namedWindow(img_name1, cv2.WINDOW_NORMAL)
    cv2.moveWindow(img_name1, 10, 10)
    cv2.namedWindow(img_name2, cv2.WINDOW_NORMAL)
    cv2.moveWindow(img_name2, 78 + width, 10)
    
    map = []
    for y in range(height):
        map.append([])
        for x in range(width):
            map[y].append(road())
    lane = 0
    for path in paths:
        lane += 1
        bx, by, bxo, byo = get_bezier(width, height, lane, BEZIER_FRACTION, points = path[0])
        for i in range(len(bx)):
            x = int(round(bx[i]))
            y = int(round(height - by[i]))
            if x >=0 and x < width and y >=0 and y < height:
                img1[y][x] = (255, 0, 0)
        cv2.imshow(img_name1, img1)
        map = get_lane_from_bezier(map, bx, by, bxo, byo, lane, stroke_width = path[1], stroke_color = path[2], image = img2, image_name = img_name2)
    for y in range(height):
        for x in range(width):
            if map[y][x].in_the_lane:
                blue = int(round(255.0 * map[y][x].broken_marking / MAX_PROB))
                green = int(round(255.0 * map[y][x].lane_center / MAX_PROB))
                red = int(round(255.0 * map[y][x].solid_marking / MAX_PROB))
                img2[height - 1 - y][x] = (blue, green, red)
    cv2.imshow(img_name2, img2)
    print 'Press "Esc" key on the image to continue...'
    while cv2.waitKey(0) & 0xFF != 0x1B: pass
    cv2.destroyAllWindows()

    map_write(map, svg_file, width, height)