Introduction
------------
A binary cube is a linear data structure that contains a sorted two dimensional dynamic array of nodes which each point to a sorted array of key/value pairs. This creates a three dimensional structure with subsequently three axes. The first two axes function as a lookup table and do not contain any keys. As the axes are of roughly similar size the structure resembles a cube.

The cube can shrink and grow as elements are added. In an ideal situation the axes are of identical size, but it doesn't significantly degrade performance if they are not perfectly matched.

To find a key a binary search is performed on the x axis to find a matching y axis, a second binary search is performed on the y axis to find a matching z axis, and a final binary search is performed to find a matching key on the z axis. Each binary search is performed in O(log (cbrt n)) time and the three searches combined should be the equivalent of an O(log n) operation.

The term sqrt stands for square root, and cbrt stands for cube root.

![binary cube visualization](https://github.com/scandum/binary_cube/blob/main/binary_cube1.png)

Insertion
---------
Inserting or removing a node is an O(cbrt n) operation. If a node falls between two Z axes the binary cube picks the lower order axis so the node is appended to the end, which is a faster operation. As a consequence a node will never be inserted on the 0 index with the exception of the floor axis.

The special case of a floor insertion can be checked at the start of the binary search, subsequent binary searches on the X, Y and Z axes can be optimized to deference the 0 index check.

Finding an index
----------------
Finding an index is an O(cbrt n) operation. In order to be able to find an index the volume of the X axes and each Y axis must be remembered. The average search time can be split in half by starting the index search at the end of the X axis if the index is greater than half the volume. With this optimization finding the first or last index is a quick process.

Finding the index on the Y axis is another O(cbrt n) operation. Finding the index on the Z axis is an O(1) operation.

Finding a key
-------------
Keys are found using a binary search. Notably the detection of equality can be skipped entirely for the X and Y axis.

In order to increase the access speed each axis can remember the floor key, which is all that is needed to perform a binary search using deferred detection of equality on the lower axes.

For each axis there is a different optimal binary search. A [quaternary binary search](https://github.com/scandum/binary_search) is a good pick for the Z axis.

Balancing a binary cube
-----------------------
Balancing a binary cube is very simple. When adding a node to the Z axis and it exceeds twice the size of the X axis the Z axis is split. If this results in the underlying Y axis exceeding twice the size of the X axis the Y axis is split. These are O(cbrt n) operations.

Similarly, when the combined volume of a Z axis and its neighbour fall below half the size of the X axis the two Z axes are merged.

Using a binary cube as a priority queue
---------------------------------------
A binary cube can be optimized to check the last index first, allowing O(1) push and pop operations.

Memory usage
------------
A binary cube has less memory overhead than a binary tree. A cube with 1,000,000 elements will have ~100 X nodes and ~10,000 Y nodes. A binary tree of the same size requires 2,000,000 leaf node pointers.

CPU usage
---------
While inserting and removing elements is an O(cbrt n) operation only half the nodes on an axis need to be moved on average. As moving memory within an array is extremely fast, insert operations on a binary cube should outperform a binary tree until the cube grows extremely large. A cube with 1 million nodes will on average require 50 nodes to be moved for an insertion, while a binary tree has to move 20 nodes using more complex operations and needing to access more uncached memory.

Fixed axis size
---------------
The lower axes in a binary cube can be given a fixed maximum size. The Z axes can be given a size of 32, the Y axes a size of 64, the X axes a size of 128, with the W axis of variable size. This type of binary cube (which can be called a tesseract for its 4 dimensions) has reasonable performance for both smaller and larger sizes.

Cubesort
--------
Cubesort uses a binary cube for its ability to partition. The Z axes are not instantly sorted using a binary insertion sort, instead cubesort waits until a Z axis before bulk sorting it using [quadsort](https://github.com/scandum/quadsort).

A binary cube can be easily converted to a 1 dimensional array and back. Cubesort is well-suited as an online or external sort.

Iterating a binary cube
-----------------------
Iterating is similar to iterating a multi-dimensional array, since the data is stored in order this is quite easy. The main problem is how to handle changes to the cube during iteration; this is not overly difficult but requires some careful consideration.

Advantages
----------
* Memory overhead similar to hash tables.
* Data is stored in order.
* Iterating a search cube is faster and easier than iterating a tree.
* Fast insertion of sorted data.
* O(cbrt n) index searches.
* Easy to balance.

Disadvantages
-------------
* High memory overhead for small data sets.

Performance
-----------
A binary cube outperforms the C++ std::map.

Source code
-----------
The source code is a bit of a proof of concept. Keys are integers but this is easily changed to strings. 
