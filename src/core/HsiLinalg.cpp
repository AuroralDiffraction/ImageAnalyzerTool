#include "core/HsiLinalg.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace iat::hsi::linalg {

QVector<double> multiply(const QVector<double>& a, const QVector<double>& b, int n, int k, int m)
{
    QVector<double> c(qsizetype(n) * m, 0.0);
    for (int i = 0; i < n; ++i)
        for (int p = 0; p < k; ++p) {
            const double v = a[qsizetype(i) * k + p];
            if (v == 0.0)
                continue;
            const double* bp = b.constData() + qsizetype(p) * m;
            double* cp = c.data() + qsizetype(i) * m;
            for (int j = 0; j < m; ++j)
                cp[j] += v * bp[j];
        }
    return c;
}

QVector<double> transpose(const QVector<double>& a, int n, int m)
{
    QVector<double> t(qsizetype(n) * m);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            t[qsizetype(j) * n + i] = a[qsizetype(i) * m + j];
    return t;
}

QVector<double> solve(QVector<double> a, QVector<double> b, int n, int m)
{
    double scale = 0.0;
    for (double v : a)
        scale = std::max(scale, std::abs(v));
    if (scale == 0.0)
        return {};
    for (int col = 0; col < n; ++col) {
        int pivot = col;
        for (int r = col + 1; r < n; ++r)
            if (std::abs(a[qsizetype(r) * n + col]) > std::abs(a[qsizetype(pivot) * n + col]))
                pivot = r;
        const double pv = a[qsizetype(pivot) * n + col];
        if (std::abs(pv) <= scale * 1e-14)
            return {};
        if (pivot != col) {
            for (int j = 0; j < n; ++j)
                std::swap(a[qsizetype(col) * n + j], a[qsizetype(pivot) * n + j]);
            for (int j = 0; j < m; ++j)
                std::swap(b[qsizetype(col) * m + j], b[qsizetype(pivot) * m + j]);
        }
        for (int r = 0; r < n; ++r) {
            if (r == col)
                continue;
            const double f = a[qsizetype(r) * n + col] / pv;
            if (f == 0.0)
                continue;
            for (int j = col; j < n; ++j)
                a[qsizetype(r) * n + j] -= f * a[qsizetype(col) * n + j];
            for (int j = 0; j < m; ++j)
                b[qsizetype(r) * m + j] -= f * b[qsizetype(col) * m + j];
        }
    }
    for (int r = 0; r < n; ++r) {
        const double d = a[qsizetype(r) * n + r];
        for (int j = 0; j < m; ++j)
            b[qsizetype(r) * m + j] /= d;
    }
    return b;
}

QVector<double> identity(int n)
{
    QVector<double> id(qsizetype(n) * n, 0.0);
    for (int i = 0; i < n; ++i)
        id[qsizetype(i) * n + i] = 1.0;
    return id;
}

QVector<double> inverse(const QVector<double>& a, int n)
{
    return solve(a, identity(n), n, n);
}

QVector<double> cholesky(const QVector<double>& a, int n)
{
    double trace = 0.0;
    for (int i = 0; i < n; ++i)
        trace += a[qsizetype(i) * n + i];
    if (!(trace > 0.0))
        return {};
    for (double ridge : {0.0, 1e-12, 1e-9, 1e-6, 1e-3}) {
        QVector<double> l(qsizetype(n) * n, 0.0);
        bool ok = true;
        for (int i = 0; i < n && ok; ++i) {
            for (int j = 0; j <= i; ++j) {
                double s = a[qsizetype(i) * n + j];
                if (i == j)
                    s += ridge * trace / n;
                for (int k = 0; k < j; ++k)
                    s -= l[qsizetype(i) * n + k] * l[qsizetype(j) * n + k];
                if (i == j) {
                    if (!(s > 0.0)) {
                        ok = false;
                        break;
                    }
                    l[qsizetype(i) * n + i] = std::sqrt(s);
                } else {
                    l[qsizetype(i) * n + j] = s / l[qsizetype(j) * n + j];
                }
            }
        }
        if (ok)
            return l;
    }
    return {};
}

void symmetricEigen(const QVector<double>& input, int n, QVector<double>& values, QVector<double>& vectors)
{
    QVector<double> a = input;
    QVector<double> v = identity(n);
    auto A = [&](int i, int j) -> double& { return a[qsizetype(i) * n + j]; };
    auto V = [&](int i, int j) -> double& { return v[qsizetype(i) * n + j]; };
    for (int sweep = 0; sweep < 100; ++sweep) {
        double off = 0.0, diag = 0.0;
        for (int i = 0; i < n; ++i) {
            diag += A(i, i) * A(i, i);
            for (int j = i + 1; j < n; ++j)
                off += A(i, j) * A(i, j);
        }
        if (off <= 1e-30 * std::max(diag, 1e-300))
            break;
        for (int p = 0; p < n; ++p)
            for (int q = p + 1; q < n; ++q) {
                const double apq = A(p, q);
                if (std::abs(apq) < 1e-300)
                    continue;
                const double theta = (A(q, q) - A(p, p)) / (2.0 * apq);
                const double t = (theta >= 0 ? 1.0 : -1.0) / (std::abs(theta) + std::sqrt(theta * theta + 1.0));
                const double c = 1.0 / std::sqrt(t * t + 1.0);
                const double s = t * c;
                for (int k = 0; k < n; ++k) {
                    const double akp = A(k, p), akq = A(k, q);
                    A(k, p) = c * akp - s * akq;
                    A(k, q) = s * akp + c * akq;
                }
                for (int k = 0; k < n; ++k) {
                    const double apk = A(p, k), aqk = A(q, k);
                    A(p, k) = c * apk - s * aqk;
                    A(q, k) = s * apk + c * aqk;
                }
                for (int k = 0; k < n; ++k) {
                    const double vkp = V(k, p), vkq = V(k, q);
                    V(k, p) = c * vkp - s * vkq;
                    V(k, q) = s * vkp + c * vkq;
                }
            }
    }
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int x, int y) { return A(x, x) > A(y, y); });
    values.resize(n);
    vectors.resize(qsizetype(n) * n);
    for (int c = 0; c < n; ++c) {
        values[c] = A(order[c], order[c]);
        for (int r = 0; r < n; ++r)
            vectors[qsizetype(r) * n + c] = V(r, order[c]);
    }
}

} // namespace iat::hsi::linalg
