## 求极限 $\lim_{x \to \infty}(\sqrt{x^2 + x} - \sqrt{x^2 - x})$

重点：共轭式

$$
\begin{split}
    \lim_{x \to \infty}(\sqrt{x^2 + x} - \sqrt{x^2 - x}) &= \lim_{x \to \infty}\frac{(\sqrt{x^2 + x} - \sqrt{x^2 - x})\cdot (\sqrt{x^2 + x} + \sqrt{x^2 - x})}{\sqrt{x^2 + x} + \sqrt{x^2 - x}}\\
    &=  \lim_{x \to \infty}\frac{x^2 + x - x^2 + x}{\sqrt{x^2 + x} + \sqrt{x^2 - x}}\\
    &=  \lim_{x \to \infty}\frac{2x}{\sqrt{x^2 + x} + \sqrt{x^2 - x}}\\
    &=  \lim_{x \to \infty}\frac{2}{\sqrt{1 + \frac{1}{x}} + \sqrt{1 - \frac{1}{x}}}\\
    &= 1
\end{split}
$$

## 求极限 $\lim_{x\to 0}\frac{(1 - \frac{1}{2}x^2)^{\frac{2}{3}}-1}{x\ln (1+x)}$

重点：等价无穷小

$$
    (1 + x)^{\alpha} - 1 \sim \alpha x\\
    x \sim \ln (1 + x)
$$

$$
   \begin{split}
    \lim_{x\to 0}\frac{(1 - \frac{1}{2}x^2)^{\frac{2}{3}}-1}{x\ln (1+x)} &= \lim_{x\to 0}\frac{\frac{2}{3}\cdot -\frac{1}{2}x^2}{x^2}\\
    &= -\frac{1}{3}
   \end{split} 
$$

## 求极限 $\lim_{x\to 0}(1 + 3\tan^2x)^{\cot^2 x}$


重点：两个重要极限

$$
   \lim_{x\to 0}(1 + x)^{\frac{1}{x}} = e
$$

$$
    \begin{split}
        \because \lim_{x\to 0}3\tan^2x \cdot \cot^2 x = 3\\
        \therefore \lim_{x\to 0}(1 + 3\tan^2x)^{\cot^2 x} &= e^3 
    \end{split}
$$

## 求极限 $\lim_{x\to \infty}(\frac{3+x}{6+x})^{\frac{x-1}{2}}$


重点：两个重要极限

$$
   \lim_{x\to 0}(1 + x)^{\frac{1}{x}} = e
$$

$$
    \begin{split}
        \lim_{x\to \infty}(\frac{3+x}{6+x})^{\frac{x-1}{2}} &= \lim_{x\to \infty}(1 - \frac{3}{6+x})^{\frac{x-1}{2}} \\
        & \overset{6 + x = t}{=} \lim_{t\to \infty}(1 - \frac{3}{t})^{\frac{t - 7}{2}}\\
        \because \lim_{t\to \infty}(-\frac{3}{t})\cdot (\frac{t - 7}{2}) = -\frac{3}{2}\\
        \therefore \lim_{t\to \infty}(1 - \frac{3}{t})^{\frac{t - 7}{2}} =\lim_{x\to \infty}(\frac{3+x}{6+x})^{\frac{x-1}{2}} = e^{-\frac{3}{2}} 
    \end{split}
$$


## 求极限 $\lim_{x\to 0}\frac{\sqrt{1 + \tan x} - \sqrt{1 + \sin x}}{x\sqrt{1 + \sin x} - x}$

重点：共轭式、重要极限与等价无穷小

$$
    x\sim \sin x \\
    (1 + x)^{\alpha} - 1 \sim \alpha x
    1 - \cos x \sim \frac{1}{2}x^2\\
$$

$$
    \begin{split}
        \lim_{x\to 0}\frac{\sqrt{1 + \tan x} - \sqrt{1 + \sin x}}{x\sqrt{1 + \sin x} - x} &= \lim_{x\to 0}\frac{(\sqrt{1 + \tan x} - \sqrt{1 + \sin x})\cdot (\sqrt{1 + \tan x} + \sqrt{1 + \sin x})}{(x\sqrt{1 + \sin^2 x} - x)\cdot (\sqrt{1 + \tan x} + \sqrt{1 + \sin x})}\\
        &= \lim_{x\to 0}\frac{1 + \tan x - 1 - \sin x}{x(\sqrt{1 + \sin^2 x } - 1)\cdot (\sqrt{1 + \tan x} + \sqrt{1 + \sin x})}\\
        &= \lim_{x\to 0}\frac{\sin x}{x} \cdot \lim_{x\to 0}\frac{\sec x - 1}{\sqrt{1 + \sin^2 x } - 1}\cdot \lim_{x\to 0}\frac{1}{\sqrt{1 + \tan x} + \sqrt{1 + \sin x}}\\
        &= 1 \cdot \lim_{x\to 0}\frac{1 - \cos x}{(1 + \sin^2 x)^{\frac{1}{2}} - 1} \cdot \frac{1}{2}\\
        &= 1 \cdot \frac{\frac{1}{2}x^2}{\frac{1}{2}\sin^2 x} \cdot \frac{1}{2}\\
        &= 1 \cdot 1 \cdot \frac{1}{2} = \frac{1}{2}\\
    \end{split}
$$

## 求极限 $\lim_{x\to e}\frac{\ln x - 1}{x - e}$

重点：等价无穷小
$$
    x \sim \ln (1 + x)
$$

$$
    \begin{split}
        \lim_{x\to e}\frac{\ln x - 1}{x - e} &\overset{x - e = t}{=} \lim_{t\to 0}\frac{\ln {(t + e)} - \ln e}{t}\\
        &= \lim_{t\to 0}\frac{\ln {(\frac{t}{e} + 1)}}{t}\\
        &= \lim_{t \to 0}\frac{\frac{t}{e}}{t} = \frac{1}{e}
    \end{split}
$$