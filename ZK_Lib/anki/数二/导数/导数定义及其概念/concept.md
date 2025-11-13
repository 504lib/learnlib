## 导数的定义

<!-- notecardId: 1762163111146 -->

$若\lim_{\Delta x \to 0}\frac{f(x + \Delta x) - f(x)}{\Delta x}存在，则称f(x)在x_0可导$

## 导数不同形式

<!-- notecardId: 1762163726141 -->

$$
\begin{split}
    f'(x_0) &= y'\mid_{x = x_0}\\
    &= \frac{dy}{dx}\mid_{x=x_0}\\
    &= \lim_{\Delta x \to 0}\frac{f(x_0 + \Delta x) - f(x_0)}{\Delta x}\\
    &= \lim_{x\to x_0}\frac{f(x) - f(x_0)}{x - x_0}
\end{split}
$$

$若以上极限不存在，则称f(x)在x_0处$_____,$若极限为无穷大，则称f(x)在x_0处 \_\_\_\_\_$

%


1. 不可导
2. 导数为无穷大


## 左导数和右导数

<!-- notecardId: 1762164274669 -->

左导数:

$$
    f'_{-}(x) = 
$$

右导数:

$$
    f'_{+}(x) = 
$$

%

$$
左导数:
    f'^{-}(x) = \lim_{\Delta x \to 0^-}\frac{f(x_0 + \Delta x) - f(x_0)}{\Delta x} = \lim_{x \to x_0^-}\frac{f(x) - f(x_0)}{x - x_0} 
$$


$$
右导数:
    f'^{+}(x)' = \lim_{\Delta x \to 0^+}\frac{f(x_0 + \Delta x) - f(x_0)}{\Delta x} = \lim_{x \to x_0^+}\frac{f(x) - f(x_0)}{x - x_0}
$$

## 区间上可导条件与导函数

<!-- notecardId: 1762164319951 -->

区间上可导：

导函数：


%

区间上可导：
$$
    f(x) 在区间I的每一点都可导
$$

导函数：
$$
    f'(x)~~~(x\in I)
$$

## 可导和左右导数存在关系

<!-- notecardId: 1762164459664 -->

$$
    可导 \iff 左右导数存在且相同
$$

## 切线方程与法线方程

<!-- notecardId: 1762164529211 -->

$$
    切线方程:y - y_0 = f'(x)(x - x_0)\\
    法线方程:y - y_0 = -\frac{1}{f'(x)}(x - x_0)
$$

## 可导与连续的关系

<!-- notecardId: 1762164697792 -->

$$
    可导\implies 连续\\
$$

## 若$f^{(n)}(x)在区间上连续，称f(x)在I上n阶连续可导$

<!-- notecardId: 1762939575051 -->

## 定理

<!-- notecardId: 1762939587773 -->

设$u,v$都是$n$阶可导
1. $(u \pm v)^{(n)} = $
2. 莱布尼兹公式：

%

1. $(u \pm v)^{(n)} = u^{(n)}\pm v^{(n)}$
2. 莱布尼兹公式：$(uv)^{(n)} = \sum_{k = 0}^{n}C_{n}^{k}u^{(n-k)}v^k$

## 高阶导数定义

<!-- notecardId: 1762939849669 -->

$$
    \begin{split}
        f^{(m)}(x_0) &= \lim_{\Delta x \to 0}\frac{f^{(n-1)}(x_0 + \Delta x) - f^{(n-1)}(x_0)}{x - x_0}\\
        & = \lim_{x \to x_0}\frac{f^{(n-1)}(x) - f^{(n-1)}(x_0)}{x - x_0}
    \end{split}
$$

## 三角函数n阶导

<!-- notecardId: 1762940115342 -->

1. $(\sin x)^{(n)} = \sin (x + n \cdot \frac{\pi}{2})$
2. $(\cos x)^{(n)} = \cos (x + n \cdot \frac{\pi}{2})$
