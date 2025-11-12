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

## w9

<!-- notecardId: 1762772000245 -->

求$y = \tan x^2$导数

%

$$
    \begin{split}
        y' &= \sec^2 x^2 \cdot 2x \\
        &= 2x\sec^2 x^2
    \end{split}
$$

## w10

<!-- notecardId: 1762773496258 -->

求$y = e^{-\frac{x}{2}}\cdot \cos 3x$导数

%

$$
    \begin{split}
        y' &= e^{-\frac{x}{2}}\cdot \cos 3x \cdot -\frac{1}{2} + e^{-\frac{x}{2}} \cdot (-\sin 3x) \cdot 3\\
        &= e^{-\frac{x}{2}} \cdot (-\frac{1}{2}\cdot \cos 3x - 3 \cdot \sin 3x)
    \end{split}
$$

## w11

<!-- notecardId: 1762773840080 -->

求$y = \arcsin\sqrt{x}$导数

%

$$
    \begin{split}
        y' &= \frac{1}{\sqrt{1 - x}}\cdot \frac{1}{2\sqrt{x}}
    \end{split}
$$


## w12

<!-- notecardId: 1762774001731 -->


求$y = (\arcsin \frac{x}{2})^2$导数

%

$$
    \begin{split}
        y' = 2\arcsin \frac{x}{2} \cdot \frac{1}{\sqrt{1 - (\frac{x}{2})^2}}
    \end{split}
$$

## w13

<!-- notecardId: 1762774102242 -->


求$y = \sqrt{1 + \ln^2 x}$导数

%

$$
    \begin{split}
        y' &= \frac{1}{2\sqrt{1 + \ln^2 x}}\cdot 2\ln x \cdot \frac{1}{x}
    \end{split}
$$

## w14

<!-- notecardId: 1762774236350 -->


求$y = \sin^n x\cos nx$导数

%

$$
    \begin{split}
        y' &= n\sin^{n-1} x\cdot \cos x \cdot \cos nx + \sin^n x\cdot (-\sin nx) \cdot n
    \end{split}
$$


## w15

<!-- notecardId: 1762774397037 -->


求$y = \sin^n x\cos nx$导数

%

$$
    \begin{split}
        y' &= n\sin^{n-1} x\cdot \cos x \cdot \cos nx + \sin^n x\cdot (-\sin nx) \cdot n
    \end{split}
$$

## w16

<!-- notecardId: 1762774421512 -->


求$y = \frac{\sqrt{1 + x} - \sqrt{1 - x}}{\sqrt{1 + x} + \sqrt{1 - x}}$导数

%

$$
    \begin{split}
        y' &= \frac{(\frac{1}{2\sqrt{1 + x}} - \frac{1}{2\sqrt{1 - x}})\cdot (\sqrt{1 + x} + \sqrt{1 - x}) - (\frac{1}{2\sqrt{1 + x}} + \frac{1}{2\sqrt{1 - x}})\cdot (\sqrt{1 + x} - \sqrt{1 - x})}{(\sqrt{\sqrt{1 + x} + \sqrt{1 - x}})^2}
    \end{split}
$$

## w17

<!-- notecardId: 1762774901809 -->

求$y = \sqrt{f^2(x) + g^2(x)} (f^2(x) + g^2(x) \neq 0)$导数

%

$$
    \begin{split}
        y' &= \frac{1}{2\sqrt{f^2(x) + g^2(x)}} \cdot (2\cdot f(x) \cdot f'(x) + 2\cdot g(x) \cdot g'(x))
    \end{split}
$$

## w18

<!-- notecardId: 1762775477982 -->

设函数$f(x)$满足以下条件：
1. $f(x + y) = f(x)\cdot f(y),对一切x,y\in R$
2. $f(x) = 1 + xg(x),而\lim_{x\to 0}g(x) = 1$

试证明$f(x)$在$R$上处处可导，且$f'(x) = f(x)$

%

$$
当x = 0时，f(y) = f(0) \cdot f(y),即f(0) = 1\\
    \begin{split}
        \lim_{\Delta x\to 0}\frac{f(x + \Delta x) - f(x)}{\Delta x} &= \lim_{\Delta x \to 0}\frac{f(x)f(\Delta x) - f(x)}{\Delta x}\\
        &= \lim_{\Delta x \to 0}\frac{f(x)[f(\Delta x ) - 1]}{\Delta x}\\
        &= \lim_{\Delta x \to 0}\frac{f(x)[1 + \Delta x g(\Delta x) - 1] }{\Delta x}\\
        &= \lim_{\Delta x \to 0}\frac{f(x)\cdot\Delta x\cdot g(\Delta x) }{\Delta x}\\
        &= \lim_{\Delta x \to 0}f(x)\cdot g(\Delta x)\\
        &= f(x)
    \end{split}
$$


## w19

<!-- notecardId: 1762940350827 -->


已知$\frac{dx}{dy} = \frac{1}{y'},求\frac{d^2x}{dy^2},\frac{d^3x}{dy^3}$

%

$$
    \begin{split}
        \frac{d^2x}{dy^2} &= \frac{d}{dy}\cdot \frac{dx}{dy}\\
        &= \frac{d}{dy}\cdot \frac{1}{y'}\\
        &= \frac{d}{dx}\cdot(\frac{1}{y'})\frac{dx}{dy}\\
        &= -\frac{y''}{(y')2}\cdot \frac{1}{y'}\\
        &= -\frac{y''}{(y')^3}
    \end{split}\\
    \begin{split}
        \frac{d^3x}{dy^3} &= \frac{d}{dy}\cdot\frac{d^2x}{dy^2}\\
        &= \frac{d}{dx}\cdot(-\frac{y''}{(y')^3})\cdot \frac{dx}{dy}\\
        &= -\frac{y'''\cdot (y')^3 - 3\cdot(y')^2\cdot (y'')^2}{(y')^6}\cdot \frac{1}{y'}\\
        &= -\frac{y'''\cdot (y')^3 - 3\cdot(y')^2\cdot (y'')^2}{(y')^5}\\
        &= -\frac{y'''\cdot y' - 3\cdot (y'')^2}{(y')^3}\\
    \end{split}
$$

## w20

<!-- notecardId: 1762944667131 -->

$y = x^2\sin 2x,求y^{(50)}$

%

$$
    (\sin x)^{(n)} = 2^n \sin (2x + n\frac{\pi}{2})\\
    \begin{split}
        (x^2\sin 2x)^{(50)}  &= \sum_{k = 0}^{n = 50}C_{50}^{k}(x^2)^{(k)}\cdot (\sin x)^{(n - k)}\\
        &= x^2 \cdot (\sin 2x)^{(50)} + 50\cdot 2x \cdot (\sin x)^{(49)} + C_{50}^{2}\cdot 2 \cdot (\sin x)^{(48)} \\
        &= -x^2 \cdot 2^{50} \cdot (\sin 2x) + 50\cdot 2x \cdot 2^{49} \cdot(\cos 2x) + C_{50}^{2}\cdot 2 \cdot 2^{48} \cdot (\sin 2x) \\
        &= 2^{50}\cdot (-x^2\sin x + 50x\cos 2x + \frac{1225}{2}\sin 2x)
    \end{split}
$$


## w21

<!-- notecardId: 1762945620888 -->


$y = x\ln x,求y^{(n)}$

%

$$
    \begin{split}
        y &= x\ln x\\
        y' &= \ln x + 1\\
        y'' &= x^{-1}\\
        y'''& = -x^{(-2)}\\
        ...\\
        y^{(n)} &= \frac{(1)^{n-2}\cdot (n-2)!}{x^{n-1}}
    \end{split}
$$