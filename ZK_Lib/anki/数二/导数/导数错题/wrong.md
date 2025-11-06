## w1

<!-- notecardId: 1762414817569 -->

$设f(x) = 10x^2,试按定义求f'(-1)$

%

$$
    \begin{split}
       f'(x)=\lim_{\Delta x \to 0}\frac{f(x + \Delta x) - f(x)}{\Delta x} &=  \lim_{\Delta x \to 0}\frac{10(x + \Delta x)^2 - x^2}{\Delta x}\\
       &= \lim_{\Delta x \to 0}10x^2 \cdot \frac{(1 + \frac{\Delta x}{x})^2-1}{\Delta x}\\
       &= \lim_{\Delta x \to 0}\frac{10x^2 \cdot 2\frac{\Delta x}{x}}{\Delta x}\\
       &= \lim_{\Delta x \to 0}20x\\
       &= 20x\\
       \therefore f'(-1) = 20 \cdot (-1) = -20
    \end{split}
$$

## w2

<!-- notecardId: 1762415483850 -->

证明$(\cos x)' = -\sin x$

%

$$
    \begin{split}
        (\cos x)' &= \lim_{\Delta x \to 0}\frac{f(x + \Delta x) - f(x)}{\Delta x}\\
        &= \lim_{\Delta x \to 0}\frac{\cos (x + \Delta x) - \cos x}{\Delta x}\\
        &= \lim_{\Delta x \to 0}\frac{-2\sin\frac{ \Delta x}{2}\cdot\sin\frac{ (2x + \Delta x)}{2}}{\Delta x}\\
        &= \lim_{\Delta x \to 0}- \frac{\sin (\frac{2x + \Delta x}{2})\cdot\sin \frac{\Delta x}{2}}{\frac{\Delta x}{2}}\\
        &= \lim_{\Delta x \to 0}-\sin (\frac{2x + \Delta x}{2})\\
        &= -\sin x
    \end{split}
$$

## w3

<!-- notecardId: 1762416347558 -->

$下列各题中均假定f'(x_0)存在,按照导数定义观察下列极限，指出A表示什么$

1. $\lim_{\Delta x \to 0}\frac{f(x_0 - \Delta x) - f(x_0)}{\Delta x} = A$
2. $\lim_{x \to 0}\frac{f(x)}{x},其中f(0) = 0,且f'(0)存在$
3. $\lim_{h \to 0}\frac{f(x_0 + h) - f(x_0 - h)}{h} = A$


%

1. 解
    $$
     \lim_{\Delta x \to 0}\frac{f(x_0 - \Delta x) - f(x_0)}{\Delta x} = -f'(x)  
    $$
2. 解
    $$
      f'(0) = \lim_{\Delta x \to 0}\frac{f(x + \Delta x) - f(x)}{\Delta x} = \lim_{\Delta x \to 0}\frac{f(\Delta x)}{\Delta x} =  \lim_{x \to 0}\frac{f(x)}{x} = A
    $$
3. 解
    $$
     \lim_{h \to 0}\frac{f(x_0 + h) - f(x_0 - h)}{h} \overset{x_0 - h = t}{\Rarr}\lim_{h \to 0}\frac{f(t + 2h) - f(t)}{h} = 2f'(h)  = A  
    $$


## w4

<!-- notecardId: 1762417540366 -->

$$
    设
    f(x) = \begin{cases}
        \frac{2}{3}x^3,x\leq 1,\\
        x^2,x>1
    \end{cases}
$$ 

$则f(x)在x = 1 处的\_\_\_$

A. 左右导数都存在
B. 左导数存在，右导数不存在
C. 左导数不存在，右导数存在
D. 左右导数都不存在

%

$$
    f'(x)=\begin{cases}
        2x^2,x\leq 1\\
        2x,x>1
    \end{cases}
$$

$$
    \begin{split}
        左极限：\lim_{x \to 1^-}\frac{f'(x) - f'(1)}{x - 1} &= \frac{2x^2 - 2}{x - 1}\\ &= \lim_{x \to 1^-}2\frac{(x-1)(x+1)}{x -1}\\ &= \lim_{x \to 1^-}2(x + 1)\\ &= 4
    \end{split}\\
    \begin{split}
       右极限：\lim_{x \to 1^+}\frac{f'(x) - f'(1)}{x - 1} &= \lim_{x \to 1^+}\frac{2x - 2}{x -1}\\
       &= \lim_{x \to 1^+}2\\
       &= 2
    \end{split}
$$

所以是B

## w5
<!-- notecardId: 1762418180722 -->

$求曲线y = \cos x上点(\frac{\pi}{3},\frac{1}{2})处的切线方程和法线方程$

%

$$
    \begin{cases}
        切线方程: y - \frac{1}{2} = -\frac{\sqrt{3}}{2}(x - \frac{\pi}{2})\\
        法线方程：y - \frac{1}{2} = \frac{2}{\sqrt{3}}(x - \frac{\pi}{2})
    \end{cases}
$$

## w6

<!-- notecardId: 1762418486455 -->

$讨论下列函数在x = 0 处的连续性与可导性$
1. $y = |sinx|$
2. $y = \begin{cases}x^2\sin \frac{1}{x},x\neq 0\\0,x = 0\end{cases}$

%

1. 连续性：
   $$
    \lim_{x \to 0^-}|\sin x| = \lim_{x \to 0^-}-\sin x = 0\\
    \lim_{x \to 0^+}|\sin x| = \lim_{x \to 0^+}\sin x = 0\\
    \therefore 连续
   $$
   可导性:
   $$
    \begin{split}
        f_{-}'(0) &= \lim_{x \to 0^-}\frac{|\sin x| - |\sin 0|}{x - 0}\\
        &= \lim_{x\to 0^-}-\frac{\sin x}{x} = -1\\
        f_{+}'(0) &= \lim_{x \to 0^+}\frac{|\sin x|}{x}\\
        &= \lim_{x \to 0^+}\frac{\sin x}{x} = 1\\
        \therefore 不可导
    \end{split}
   $$

2. 连续性:
   $$
    \begin{split}
        &\lim_{x \to 0}x^2\sin \frac{1}{x} = 0\\
        &\therefore 连续
    \end{split}
   $$
   可导性:
   $$
    \begin{split}
       &\lim_{x \to 0^-}\frac{x^2\sin \frac{1}{x}}{x} = \lim_{x \to 0^+}\frac{x^2\sin \frac{1}{x}}{x}\\
       &\therefore 可导
    \end{split}
   $$

## w7

<!-- notecardId: 1762419599878 -->

$$
    设函数\begin{cases}
        x^2 , x \leq 1,\\
        ax + b, x > 1
    \end{cases}\\
    为了使函数f(x)在x = 1处连续且可导,a,b应取什么值
$$

%

$$
    连续证明:\\
    \begin{cases}
        \lim_{x\to 1^-}x^2 = 1\\
        \lim_{x \to 1^-}ax+b = a + b
    \end{cases}\\
    已知f(x)在x  = 1处连续，则a+b = 1\\
    可导证明:\\
    \begin{cases}
        f_{-}'(x)\lim_{x \to 1^-}\frac{x^2 - 1}{x - 1} = x + 1 = 2\\
        f_(+)'(x)\lim_{x \to 1^+}\frac{ax - a}{x - 1} = a
    \end{cases}\\
    已知f(x)在x = 1处可导，则a = 2\\
    \therefore\begin{cases}
        a = 2\\
        a + b = 1
    \end{cases}\Rarr\begin{cases}
        a = 2\\
        b = -1
    \end{cases}
$$

## w8

<!-- notecardId: 1762421482024 -->

$已知f(x) = \begin{cases}-x,x<0\\x,x\ge 0\end{cases},求f_{+}'(0)及f_{-}'(0),又f'(0)是否存在$

%

$$
    \begin{cases}
        f_{-}'(0) = \lim_{x \to 0^-}-\frac{-x}{x} = -1\\
        f_{+}'(0) = \lim_{x \to 0^+}\frac{x}{x} = 1
    \end{cases}\\
    \therefore f_{-}'(0)与f_{+}'(0)存在，但f'(0)不存在
$$