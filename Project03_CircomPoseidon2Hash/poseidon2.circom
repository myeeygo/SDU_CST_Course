pragma circom 2.0.0;
include "./poseidon2_constants.circom";

// 5次幂S盒 (文档1-68提到d≥3且与p-1互质)
template Sigma() {
    signal input in;
    signal output out;
    signal in2, in4;
    in2 <== in * in;       // x²
    in4 <== in2 * in2;     // x⁴
    out <== in4 * in;      // x⁵
}

// 轮常量加法 (文档1-120提到内部轮仅第一个元素有常量)
template Ark(t, C, r, isInternal) {
    signal input in[t];
    signal output out[t];
    for (var i = 0; i < t; i++) {
        if (isInternal == 1) {
            out[i] <== (i == 0) ? in[i] + C[r] : in[i];  // 内部轮仅第一个元素加常量
        } else {
            out[i] <== in[i] + C[r * t + i];             // 外部轮全元素加常量
        }
    }
}

// 外部轮线性层Mε (文档1-96, 1-98)
template MixE(t, M) {
    signal input in[t];
    signal output out[t];
    var lc;
    for (var i = 0; i < t; i++) {
        lc = 0;
        for (var j = 0; j < t; j++) {
            lc += M[j][i] * in[j];  // 矩阵乘法: out[i] = sum(M[j][i] * in[j])
        }
        out[i] <== lc;
    }
}

// 内部轮线性层MI (文档1-103)
template MixI(t, M) {
    signal input in[t];
    signal output out[t];
    var sum = 0;
    for (var i = 0; i < t; i++) {
        sum += in[i];  // 预计算输入总和 (文档1-108)
    }
    for (var i = 0; i < t; i++) {
        out[i] <== (M[i] - 1) * in[i] + sum;  // 优化计算: (μi-1)*xi + sum
    }
}

// 输出混合层 (提取指定索引的输出)
template MixLast(t, M, idx) {
    signal input in[t];
    signal output out;
    var lc = 0;
    for (var j = 0; j < t; j++) {
        lc += M[j][idx] * in[j];
    }
    out <== lc;
}

// Poseidon2主模板 (支持多输入多输出)
template Poseidon2Ex(nInputs, nOuts) {
    signal input inputs[nInputs];
    signal input initialState;
    signal output out[nOuts];

    // 轮次参数 (文档1-140表1)
    var N_ROUNDS_P[16] = [56, 57, 56, 60, 60, 63, 64, 63, 60, 66, 60, 65, 70, 60, 64, 68];
    var t = nInputs + 1;  // 状态大小 = 输入数 + 容量
    var nRoundsF = 8;     // 外部轮总数
    var nRoundsP = N_ROUNDS_P[t - 2];  // 内部轮数

    // 常量与矩阵 (从poseidon2_constants.circom导入)
    var C_EXT[t * (nRoundsF/2)] = POSEIDON2_C_EXT(t);  // 外部轮常量
    var C_INT[nRoundsP] = POSEIDON2_C_INT(t);          // 内部轮常量
    var M_E[t][t] = POSEIDON2_M_E(t);                  // 外部轮矩阵Mε
    var M_I[t] = POSEIDON2_M_I(t);                     // 内部轮矩阵MI (对角线元素μi)
    var M_FINAL[t][t] = POSEIDON2_M_FINAL(t);          // 最终混合矩阵

    // 组件声明
    component initialMix = MixE(t, M_E);  // 初始线性层 (文档1-130)
    component arkExternal[nRoundsF/2 + nRoundsF/2];
    component sigmaExternal[nRoundsF][t];
    component mixExternal[nRoundsF - 1];
    component arkInternal[nRoundsP];
    component sigmaInternal[nRoundsP];
    component mixInternal[nRoundsP];
    component mixLast[nOuts];

    // 初始化状态 (输入 + 初始状态)
    signal state[t];
    for (var j = 0; j < t; j++) {
        state[j] <== (j == 0) ? initialState : inputs[j - 1];
    }

    // 初始线性层应用 (文档1-120)
    for (var j = 0; j < t; j++) {
        initialMix.in[j] <== state[j];
    }

    // 前半外部轮 (R_F/2)
    var currentState = initialMix.out;
    for (var r = 0; r < nRoundsF/2; r++) {
        // 轮常量
        arkExternal[r] = Ark(t, C_EXT, r, 0);
        for (var j = 0; j < t; j++) {
            arkExternal[r].in[j] <== currentState[j];
        }

        // 全S盒
        for (var j = 0; j < t; j++) {
            sigmaExternal[r][j] = Sigma();
            sigmaExternal[r][j].in <== arkExternal[r].out[j];
        }

        // 混合层 (最后一轮外部轮不混合，衔接内部轮)
        if (r < nRoundsF/2 - 1) {
            mixExternal[r] = MixE(t, M_E);
            for (var j = 0; j < t; j++) {
                mixExternal[r].in[j] <== sigmaExternal[r][j].out;
            }
            currentState = mixExternal[r].out;
        } else {
            currentState = sigmaExternal[r][0].out;  // 最后一轮外部轮输出直接进入内部轮
        }
    }

    // 内部轮 (R_P)
    for (var r = 0; r < nRoundsP; r++) {
        // 轮常量 (仅第一个元素)
        arkInternal[r] = Ark(t, C_INT, r, 1);
        for (var j = 0; j < t; j++) {
            arkInternal[r].in[j] <== (j == 0) ? currentState : state[j];
        }

        // 部分S盒 (仅第一个元素)
        sigmaInternal[r] = Sigma();
        sigmaInternal[r].in <== arkInternal[r].out[0];

        // 内部混合层
        mixInternal[r] = MixI(t, M_I);
        for (var j = 0; j < t; j++) {
            mixInternal[r].in[j] <== (j == 0) ? sigmaInternal[r].out : arkInternal[r].out[j];
        }
        currentState = mixInternal[r].out[0];
        for (var j = 1; j < t; j++) {
            state[j] <== mixInternal[r].out[j];
        }
    }

    // 后半外部轮 (R_F/2)
    for (var r = nRoundsF/2; r < nRoundsF; r++) {
        // 轮常量
        arkExternal[r] = Ark(t, C_EXT, r - nRoundsF/2, 0);
        for (var j = 0; j < t; j++) {
            arkExternal[r].in[j] <== (j == 0) ? currentState : state[j];
        }

        // 全S盒
        for (var j = 0; j < t; j++) {
            sigmaExternal[r][j] = Sigma();
            sigmaExternal[r][j].in <== arkExternal[r].out[j];
        }

        // 混合层
        if (r < nRoundsF - 1) {
            mixExternal[r] = MixE(t, M_E);
            for (var j = 0; j < t; j++) {
                mixExternal[r].in[j] <== sigmaExternal[r][j].out;
            }
            currentState = mixExternal[r].out[0];
            for (var j = 1; j < t; j++) {
                state[j] <== mixExternal[r].out[j];
            }
        }
    }

    // 最终输出混合
    for (var i = 0; i < nOuts; i++) {
        mixLast[i] = MixLast(t, M_FINAL, i);
        for (var j = 0; j < t; j++) {
            mixLast[i].in[j] <== (j == 0) ? currentState : state[j];
        }
        out[i] <== mixLast[i].out;
    }
}

// 单输出封装模板
template Poseidon2(nInputs) {
    signal input inputs[nInputs];
    signal output out;
    component p2Ex = Poseidon2Ex(nInputs, 1);
    p2Ex.initialState <== 0;  // 初始状态设为0
    for (var i = 0; i < nInputs; i++) {
        p2Ex.inputs[i] <== inputs[i];
    }
    out <== p2Ex.out[0];
}

template Main() {
    signal input preimage; // 私有输入
    signal input hash;    // 公共输入

    component posei = Poseidon2(1);
    posei.inputs[0] <== preimage;

    hash === posei.out;
}

component main = Main();