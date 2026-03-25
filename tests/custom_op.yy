infix operator +- 150 lassoc;

class Point {
    func init(x: int = 0, y: int = 0) {
        this.x = x;
        this.y = y;
    }
};

operator Point +- Point (a, b) {
    return Point(a.x - b.x, a.y - b.y);
}