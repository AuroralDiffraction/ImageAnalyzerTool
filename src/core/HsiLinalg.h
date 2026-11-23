#pragma once

#include <QVector>

namespace iat::hsi::linalg {

// Small dense linear algebra for the hyperspectral code (matrices of a few
// hundred elements at most: 16 × 16 covariances, 3 × 16 colour matrices,
// 201 × 16 spectral responses). Matrices are row-major QVector<double> with
// the sizes passed alongside.

// C (n × m) = A (n × k) · B (k × m)
QVector<double> multiply(const QVector<double>& a, const QVector<double>& b, int n, int k, int m);
// Aᵀ for A (n × m): m × n
QVector<double> transpose(const QVector<double>& a, int n, int m);
// Solves A X = B for X, A (n × n), B (n × m), by Gaussian elimination with
// partial pivoting. Returns an empty vector if A is singular.
QVector<double> solve(QVector<double> a, QVector<double> b, int n, int m);
// Inverse of A (n × n); empty if singular.
QVector<double> inverse(const QVector<double>& a, int n);
// Lower-triangular L with A = L Lᵀ for a symmetric positive definite A
// (n × n). A tiny ridge is added to the diagonal if needed; empty on failure.
QVector<double> cholesky(const QVector<double>& a, int n);
// Eigen-decomposition of a symmetric A (n × n) by cyclic Jacobi rotations.
// `values` in descending order, `vectors` n × n with eigenvector i in
// column i.
void symmetricEigen(const QVector<double>& a, int n, QVector<double>& values, QVector<double>& vectors);
// n × n identity
QVector<double> identity(int n);

} // namespace iat::hsi::linalg
